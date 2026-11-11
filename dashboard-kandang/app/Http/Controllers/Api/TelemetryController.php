<?php

namespace App\Http\Controllers\Api;

use App\Http\Controllers\Controller;
use App\Models\Telemetry;
use Illuminate\Http\Request;

class TelemetryController extends Controller
{
    public function index()
    {
        return response()->json([
            'success' => true,
            'data' => Telemetry::latest()->limit(100)->get(),
        ]);
    }

    public function store(Request $request)
    {
        $validated = $request->validate([
            'device_id' => ['required', 'string', 'max:100'],
            'temperature' => ['required', 'numeric'],
            'humidity' => ['required', 'numeric', 'between:0,100'],
        ]);

        $telemetry = Telemetry::create($validated);

        return response()->json([
            'success' => true,
            'message' => 'Telemetry stored successfully',
            'data' => $telemetry,
        ], 201);
    }
}
