<?php

namespace Tests\Feature;

use App\Models\Telemetry;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Tests\TestCase;

class ExampleTest extends TestCase
{
    use RefreshDatabase;

    /**
     * A basic test example.
     */
    public function test_the_application_returns_a_successful_response(): void
    {
        $response = $this->get('/');

        $response->assertStatus(200);
    }

    public function test_telemetry_can_be_retrieved_with_get(): void
    {
        $telemetry = Telemetry::create([
            'device_id' => 'coop-1',
            'temperature' => 28.5,
            'humidity' => 70,
        ]);

        $this->getJson('/api/telemetry')
            ->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('data.0.id', $telemetry->id)
            ->assertJsonPath('data.0.device_id', 'coop-1');
    }

    public function test_telemetry_can_still_be_stored_with_post(): void
    {
        $this->postJson('/api/telemetry', [
            'device_id' => 'coop-1',
            'temperature' => 28.5,
            'humidity' => 70,
        ])
            ->assertCreated()
            ->assertJsonPath('success', true)
            ->assertJsonPath('data.device_id', 'coop-1');
    }
}
