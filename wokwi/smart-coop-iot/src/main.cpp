
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// WIFI CONFIGURATION
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const char* API_URL =
    "http://host.wokwi.internal:8000/api/telemetry";

const char* DEVICE_ID = "ESP32-KANDANG-001";

// PIN CONFIGURATION
#define DHT_PIN             15
#define DHT_TYPE            DHT22

#define MQ2_PIN             34
#define POT_PIN             35

#define LCD_SDA_PIN         21
#define LCD_SCL_PIN         22

#define SERVO_PIN           18
#define RELAY_PIN           19

#define BUZZER_PIN          23
#define GAS_LED_PIN         25

#define MODE_BUTTON_PIN     26

#define RGB_RED_PIN         16
#define RGB_GREEN_PIN       17
#define RGB_BLUE_PIN        27

// HARDWARE OBJECTS
DHT dht(DHT_PIN, DHT_TYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo ventilationServo;

// RELAY CONFIGURATION
// Active LOW
const int RELAY_ON = LOW;
const int RELAY_OFF = HIGH;

// MQ2 BASIC PRACTICE CONFIGURATION
// Baseline calibration.
const int GAS_CALIBRATION_SAMPLES = 20;

// Gas warning and alarm thresholds.
const int GAS_WARNING_DELTA = 100;
const int GAS_ALARM_DELTA = 200;

// Sensor sampling interval.
const unsigned long GAS_SAMPLE_INTERVAL = 500;

// Gas status.
enum GasStatus {
    GAS_NORMAL,
    GAS_WARNING,
    GAS_DANGER
};

GasStatus gasStatus = GAS_NORMAL;

// TEMPERATURE CONFIGURATION
const float TEMP_HYSTERESIS = 1.0;

// TIMING CONFIGURATION
const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long LCD_INTERVAL = 500;
const unsigned long SERIAL_INTERVAL = 2000;

const unsigned long WIFI_RECONNECT_INTERVAL = 10000;
const unsigned long API_INTERVAL = 10000;

const unsigned long LONG_PRESS_DURATION = 800;

// SYSTEM STATE
enum OperatingMode {
    AUTO_MODE,
    MANUAL_MODE
};

OperatingMode currentMode = AUTO_MODE;

bool fanOn = false;
bool manualFanOn = false;

bool gasAlarm = false;
bool dhtError = false;
bool buzzerActive = false;

float temperature = 0.0;
float humidity = 0.0;
float setpoint = 30.0;

int gasADC = 0;
int gasBaseline = 0;
int gasDelta = 0;

int gasWarningThreshold = 0;
int gasAlarmThreshold = 0;

int potADC = 0;
int ventilationAngle = 0;

bool gasCalibrated = false;

// TIMER VARIABLES
unsigned long lastSensorRead = 0;
unsigned long lastGasSample = 0;
unsigned long lastLCDUpdate = 0;
unsigned long lastSerialPrint = 0;
unsigned long lastWiFiReconnect = 0;
unsigned long lastAPIRequest = 0;

// BUTTON STATE
bool buttonWasPressed = false;
unsigned long buttonPressStart = 0;

// RGB LED CONFIGURATION
// Common Anode: LOW = ON
void setRGB(bool red, bool green, bool blue) {

    digitalWrite(RGB_RED_PIN, red ? LOW : HIGH);
    digitalWrite(RGB_GREEN_PIN, green ? LOW : HIGH);
    digitalWrite(RGB_BLUE_PIN, blue ? LOW : HIGH);
}

// MQ2 SENSOR CALIBRATION
void calibrateGasSensor() {

    Serial.println();
    Serial.println("====================================");
    Serial.println("       MQ2 CALIBRATION");
    Serial.println("====================================");

    Serial.println("Keep MQ2 at normal condition.");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQ2 Calibration");

    long total = 0;

    for (int i = 0; i < GAS_CALIBRATION_SAMPLES; i++) {

        int reading = analogRead(MQ2_PIN);

        total += reading;

        Serial.print("Sample ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.print(GAS_CALIBRATION_SAMPLES);

        Serial.print(" | ADC: ");
        Serial.println(reading);

        lcd.setCursor(0, 1);
        lcd.print("Sample: ");
        lcd.print(i + 1);
        lcd.print("/");
        lcd.print(GAS_CALIBRATION_SAMPLES);

        delay(100);
    }

    gasBaseline = total / GAS_CALIBRATION_SAMPLES;

    gasADC = gasBaseline;
    gasDelta = 0;

    gasWarningThreshold =
        gasBaseline + GAS_WARNING_DELTA;

    gasAlarmThreshold =
        gasBaseline + GAS_ALARM_DELTA;

    gasCalibrated = true;

    Serial.println("------------------------------------");

    Serial.print("MQ2 Baseline: ");
    Serial.println(gasBaseline);

    Serial.print("Warning ADC : ");
    Serial.println(gasWarningThreshold);

    Serial.print("Alarm ADC   : ");
    Serial.println(gasAlarmThreshold);

    Serial.println("Calibration completed.");

    Serial.println("====================================");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibration OK");

    lcd.setCursor(0, 1);
    lcd.print("Base:");
    lcd.print(gasBaseline);

    delay(1000);
}

// WIFI CONNECTION
void connectWiFi() {

    Serial.println();
    Serial.println("Connecting to WiFi...");

    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startAttempt = millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - startAttempt < 10000
    ) {

        delay(500);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {

        Serial.println("WiFi Connected!");

        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

    } else {

        Serial.println("WiFi Connection Failed!");
    }
}

// WIFI RECONNECTION
void maintainWiFi() {

    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    if (
        millis() - lastWiFiReconnect >=
        WIFI_RECONNECT_INTERVAL
    ) {

        lastWiFiReconnect = millis();

        Serial.println("Reconnecting WiFi...");

        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
}

// READ POTENTIOMETER
// Setpoint: 20 - 40 Celsius
void readPotentiometer() {

    potADC = analogRead(POT_PIN);

    setpoint =
        map(potADC, 0, 4095, 2000, 4000) / 100.0;
}

// READ DHT22
void readDHTSensor() {

    float newTemperature = dht.readTemperature();
    float newHumidity = dht.readHumidity();

    if (
        isnan(newTemperature) ||
        isnan(newHumidity)
    ) {

        dhtError = true;

        Serial.println("WARNING: DHT22 SENSOR ERROR!");

        return;
    }

    temperature = newTemperature;
    humidity = newHumidity;

    dhtError = false;
}

// UPDATE MQ2 GAS STATUS
void updateGasStatus() {

    if (!gasCalibrated) {
        return;
    }

    // Read analog sensor.
    gasADC = analogRead(MQ2_PIN);

    // Calculate difference from baseline.
    gasDelta = gasADC - gasBaseline;

    // Determine gas status.
    if (gasADC >= gasAlarmThreshold) {

        gasStatus = GAS_DANGER;

    } else if (gasADC >= gasWarningThreshold) {

        gasStatus = GAS_WARNING;

    } else {

        gasStatus = GAS_NORMAL;
    }

    // Only DANGER activates the gas alarm.
    gasAlarm = (gasStatus == GAS_DANGER);
}

// TEMPERATURE CONTROL
void controlTemperature() {

    // Gas danger has highest priority.
    if (gasStatus == GAS_DANGER) {

        fanOn = true;

        return;
    }

    // Fail-safe if DHT22 fails.
    if (dhtError) {

        fanOn = true;

        return;
    }

    // Manual mode.
    if (currentMode == MANUAL_MODE) {

        fanOn = manualFanOn;

        return;
    }

    // Automatic mode with hysteresis.
    if (temperature >= setpoint + TEMP_HYSTERESIS) {

        fanOn = true;

    } else if (
        temperature <= setpoint - TEMP_HYSTERESIS
    ) {

        fanOn = false;
    }
}

// ACTUATOR CONTROL
void updateActuators() {

    // Relay control.
    digitalWrite(
        RELAY_PIN,
        fanOn ? RELAY_ON : RELAY_OFF
    );

    // Servo ventilation.
    ventilationAngle = fanOn ? 90 : 0;

    ventilationServo.write(ventilationAngle);

    // Gas warning LED.
    digitalWrite(
        GAS_LED_PIN,
        gasAlarm ? HIGH : LOW
    );

    // Buzzer only activates during gas danger.
    if (gasAlarm && !buzzerActive) {

        tone(BUZZER_PIN, 2000);

        buzzerActive = true;

    } else if (!gasAlarm && buzzerActive) {

        noTone(BUZZER_PIN);

        buzzerActive = false;
    }

    // RGB status indicator.
    if (gasStatus == GAS_DANGER) {

        // Red: danger.
        setRGB(true, false, false);

    } else if (gasStatus == GAS_WARNING) {

        // Yellow: warning.
        setRGB(true, true, false);

    } else if (dhtError) {

        // Blue: DHT22 error.
        setRGB(false, false, true);

    } else if (fanOn) {

        // Yellow: ventilation active.
        setRGB(true, true, false);

    } else {

        // Green: normal.
        setRGB(false, true, false);
    }
}

// BUTTON CONTROL
// Short press: AUTO / MANUAL
// Long press in MANUAL: Fan ON / OFF
void handleModeButton() {

    bool buttonPressed =
        digitalRead(MODE_BUTTON_PIN) == LOW;

    if (buttonPressed && !buttonWasPressed) {

        buttonPressStart = millis();

        buttonWasPressed = true;
    }

    if (!buttonPressed && buttonWasPressed) {

        unsigned long pressDuration =
            millis() - buttonPressStart;

        buttonWasPressed = false;

        if (pressDuration >= LONG_PRESS_DURATION) {

            if (currentMode == MANUAL_MODE) {

                manualFanOn = !manualFanOn;

                Serial.print("Manual Fan: ");

                Serial.println(
                    manualFanOn ? "ON" : "OFF"
                );

            } else {

                Serial.println(
                    "Switch to MANUAL mode first!"
                );
            }

        } else {

            if (currentMode == AUTO_MODE) {

                currentMode = MANUAL_MODE;

                Serial.println("Mode changed to MANUAL");

            } else {

                currentMode = AUTO_MODE;

                Serial.println("Mode changed to AUTO");
            }
        }
    }
}

// LCD DISPLAY
void updateLCD() {

    static bool displayPage = false;

    displayPage = !displayPage;

    lcd.clear();

    if (!displayPage) {

        // Page 1: Temperature and humidity.
        lcd.setCursor(0, 0);

        lcd.print("T:");
        lcd.print(temperature, 1);
        lcd.print("C H:");
        lcd.print(humidity, 0);
        lcd.print("%");

        // Page 2: Gas status.
        lcd.setCursor(0, 1);

        lcd.print("Gas:");

        if (gasStatus == GAS_NORMAL) {

            lcd.print("NORMAL");

        } else if (gasStatus == GAS_WARNING) {

            lcd.print("WASPADA");

        } else {

            lcd.print("BAHAYA");
        }

    } else {

        // Operating mode.
        lcd.setCursor(0, 0);

        lcd.print("Mode:");

        lcd.print(
            currentMode == AUTO_MODE
                ? "AUTO"
                : "MANUAL"
        );

        // Fan and setpoint.
        lcd.setCursor(0, 1);

        lcd.print("Fan:");

        lcd.print(fanOn ? "ON " : "OFF");

        lcd.print(" SP:");

        lcd.print(setpoint, 1);
    }
}

// SERIAL MONITOR
void printSystemStatus() {

    Serial.println();
    Serial.println("========== SENSOR READING ==========");

    Serial.print("Temperature : ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Humidity    : ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("MQ2 ADC     : ");
    Serial.println(gasADC);

    Serial.print("MQ2 Baseline: ");
    Serial.println(gasBaseline);

    Serial.print("MQ2 Delta   : ");
    Serial.println(gasDelta);

    Serial.print("MQ2 Warning : ");
    Serial.println(gasWarningThreshold);

    Serial.print("MQ2 Danger  : ");
    Serial.println(gasAlarmThreshold);

    Serial.print("MQ2 Voltage : ");

    Serial.print(
        (gasADC * 3300.0) / 4095.0,
        0
    );

    Serial.println(" mV");

    Serial.print("Gas Status  : ");

    if (gasStatus == GAS_NORMAL) {

        Serial.println("NORMAL");

    } else if (gasStatus == GAS_WARNING) {

        Serial.println("WASPADA");

    } else {

        Serial.println("BAHAYA");
    }

    Serial.print("Gas Alarm   : ");

    Serial.println(
        gasAlarm ? "ON" : "OFF"
    );

    Serial.print("Pot ADC     : ");
    Serial.println(potADC);

    Serial.print("Setpoint    : ");
    Serial.print(setpoint);
    Serial.println(" C");

    Serial.println("===================================");

    Serial.println();

    Serial.println("========== ACTUATOR STATUS ==========");

    Serial.print("Mode        : ");

    Serial.println(
        currentMode == AUTO_MODE
            ? "AUTO"
            : "MANUAL"
    );

    Serial.print("Fan         : ");

    Serial.println(fanOn ? "ON" : "OFF");

    Serial.print("Relay GPIO  : ");

    Serial.println(
        digitalRead(RELAY_PIN) == HIGH
            ? "HIGH"
            : "LOW"
    );

    Serial.print("Ventilation : ");

    Serial.println(
        fanOn ? "OPEN" : "CLOSED"
    );

    Serial.print("Servo Angle : ");
    Serial.println(ventilationAngle);

    Serial.print("Gas LED     : ");

    Serial.println(
        gasAlarm ? "ON" : "OFF"
    );

    Serial.print("Buzzer      : ");

    Serial.println(
        buzzerActive ? "ON" : "OFF"
    );

    Serial.println("=====================================");
}

// SEND TELEMETRY TO LARAVEL API
void sendTelemetry() {

    if (WiFi.status() != WL_CONNECTED) {

        Serial.println(
            "API skipped: WiFi disconnected."
        );

        return;
    }

    if (dhtError) {

        Serial.println(
            "API skipped: Invalid DHT22 reading."
        );

        return;
    }

    HTTPClient http;

    http.begin(API_URL);

    http.addHeader(
        "Content-Type",
        "application/json"
    );

    String payload = "{";

    payload += "\"device_id\":\"";
    payload += DEVICE_ID;
    payload += "\",";

    payload += "\"temperature\":";
    payload += String(temperature, 2);
    payload += ",";

    payload += "\"humidity\":";
    payload += String(humidity, 2);

    payload += "}";

    Serial.println();
    Serial.println("Sending telemetry to Laravel...");

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {

        Serial.print("HTTP Response: ");
        Serial.println(httpResponseCode);

        String response = http.getString();

        Serial.println(response);

    } else {

        Serial.print("HTTP Error: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

// SETUP
void setup() {

    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("======================================");
    Serial.println("         SMART COOP IoT SYSTEM");
    Serial.println("======================================");

    // Initialize I2C.
    Wire.begin(
        LCD_SDA_PIN,
        LCD_SCL_PIN
    );

    // Initialize LCD.
    lcd.init();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print("SMART COOP IoT");

    lcd.setCursor(0, 1);
    lcd.print("Initializing...");

    // Initialize DHT22.
    dht.begin();

    // Initialize GPIO.
    pinMode(MQ2_PIN, INPUT);
    pinMode(POT_PIN, INPUT);

    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(GAS_LED_PIN, OUTPUT);

    pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

    pinMode(RGB_RED_PIN, OUTPUT);
    pinMode(RGB_GREEN_PIN, OUTPUT);
    pinMode(RGB_BLUE_PIN, OUTPUT);

    // Configure ADC.
    analogReadResolution(12);

    analogSetPinAttenuation(
        MQ2_PIN,
        ADC_11db
    );

    analogSetPinAttenuation(
        POT_PIN,
        ADC_11db
    );

    // Initialize actuators in OFF state.
    digitalWrite(RELAY_PIN, RELAY_OFF);

    digitalWrite(GAS_LED_PIN, LOW);

    noTone(BUZZER_PIN);

    digitalWrite(BUZZER_PIN, LOW);

    buzzerActive = false;

    setRGB(false, true, false);

    // Initialize servo.
    ventilationServo.setPeriodHertz(50);

    ventilationServo.attach(
        SERVO_PIN,
        500,
        2400
    );

    ventilationServo.write(0);

    // Initialize WiFi.
    connectWiFi();

    // Calibrate MQ2.
    // Keep MQ2 at normal condition during calibration.
    calibrateGasSensor();

    // Initialize timers.
    lastSensorRead = millis();
    lastGasSample = millis();
    lastLCDUpdate = millis();
    lastSerialPrint = millis();
    lastAPIRequest = millis();

    Serial.println();
    Serial.println("System initialization completed.");
    Serial.println("Gas detection: READY");

    lcd.clear();
}

// MAIN LOOP
void loop() {

    unsigned long currentMillis = millis();

    // 1. Maintain WiFi.
    maintainWiFi();

    // 2. Handle AUTO / MANUAL button.
    handleModeButton();

    // 3. Read potentiometer.
    readPotentiometer();

    // 4. Read DHT22 periodically.
    if (
        currentMillis - lastSensorRead >=
        SENSOR_INTERVAL
    ) {

        lastSensorRead = currentMillis;

        readDHTSensor();
    }

    // 5. Read MQ2 periodically.
    if (
        currentMillis - lastGasSample >=
        GAS_SAMPLE_INTERVAL
    ) {

        lastGasSample = currentMillis;

        updateGasStatus();
    }

    // 6. Control temperature and gas.
    controlTemperature();

    // 7. Update actuators.
    updateActuators();

    // 8. Update LCD.
    if (
        currentMillis - lastLCDUpdate >=
        LCD_INTERVAL
    ) {

        lastLCDUpdate = currentMillis;

        updateLCD();
    }

    // 9. Print Serial Monitor.
    if (
        currentMillis - lastSerialPrint >=
        SERIAL_INTERVAL
    ) {

        lastSerialPrint = currentMillis;

        printSystemStatus();
    }

    // 10. Send telemetry to Laravel.
    if (
        currentMillis - lastAPIRequest >=
        API_INTERVAL
    ) {

        lastAPIRequest = currentMillis;

        sendTelemetry();
    }
}
