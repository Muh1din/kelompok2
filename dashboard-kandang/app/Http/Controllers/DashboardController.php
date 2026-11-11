<?php

namespace App\Http\Controllers;

use App\Models\Telemetry;

class DashboardController extends Controller
{
    public function index()
    {
        $latestTelemetry = Telemetry::latest()->first();

        $totalTelemetry = Telemetry::count();

        $history = Telemetry::latest()
            ->paginate(10);

        $chartHistory = Telemetry::latest()
            ->take(50)
            ->get()
            ->reverse()
            ->values();

        return view('dashboard', compact(
            'latestTelemetry',
            'totalTelemetry',
            'history',
            'chartHistory'
        ));
    }
}
