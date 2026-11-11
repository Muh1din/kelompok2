<?php

use App\Http\Controllers\Api\TelemetryController;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Route;

Route::get('/user', function (Request $request) {
    return $request->user();
})->middleware('auth:sanctum');


Route::get('/health', function () {
    return response()->json([
        'success' => true,
        'message' => 'Laravel IoT API is running',
    ]);
});

Route::get('/telemetry', [TelemetryController::class, 'index']);
Route::post('/telemetry', [TelemetryController::class, 'store']);