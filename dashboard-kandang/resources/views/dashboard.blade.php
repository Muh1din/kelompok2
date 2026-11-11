<!DOCTYPE html>
<html lang="id">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Coop Monitoring</title>
    @vite(['resources/css/app.css', 'resources/js/app.js'])
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>

<body class="bg-slate-50 text-slate-800 antialiased">

    @php
        $isOnline = $latestTelemetry && $latestTelemetry->created_at->gt(now()->subSeconds(90));

        $labels = $history->map(fn($item) => $item->created_at->format('H:i:s'));
        $temperatures = $history->pluck('temperature');
        $humidities = $history->pluck('humidity');
    @endphp

    <div class="min-h-screen md:flex">

        <!-- Sidebar -->
        <aside class="w-full md:w-64 bg-slate-950 text-white md:min-h-screen p-6">
            <div class="flex items-center gap-3 mb-10">
                <div class="w-11 h-11 rounded-xl bg-emerald-500 flex items-center justify-center font-bold text-xl">
                    SC
                </div>
                <div>
                    <h1 class="font-bold text-lg">Smart Coop</h1>
                    <p class="text-xs text-slate-400">Monitoring System</p>
                </div>
            </div>

            <p class="text-xs uppercase tracking-wider text-slate-500 mb-3">
                Main Menu
            </p>

            <nav class="space-y-2">
                <a href="{{ route('dashboard') }}"
                    class="flex items-center gap-3 px-4 py-3 rounded-xl bg-emerald-500 text-white font-medium">
                    <span>▦</span> Dashboard
                </a>

                <a href="#monitoring"
                    class="flex items-center gap-3 px-4 py-3 rounded-xl text-slate-300 hover:bg-slate-800">
                    <span>◉</span> Monitoring
                </a>

                <a href="#history"
                    class="flex items-center gap-3 px-4 py-3 rounded-xl text-slate-300 hover:bg-slate-800">
                    <span>◷</span> Riwayat Data
                </a>
            </nav>

            <div class="mt-12 rounded-2xl bg-slate-900 p-4">
                <p class="text-sm font-semibold">IoT Device</p>
                <p class="text-xs text-slate-400 mt-1">ESP32 Development Board</p>
                <div class="flex items-center gap-2 mt-4 text-sm">
                    <span class="w-2 h-2 rounded-full {{ $isOnline ? 'bg-emerald-400' : 'bg-red-400' }}"></span>
                    {{ $isOnline ? 'Device Online' : 'Device Offline' }}
                </div>
            </div>
        </aside>

        <!-- Main Content -->
        <main class="flex-1 min-w-0 p-5 md:p-8">

            <!-- Header -->
            <header class="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4 mb-8">
                <div>
                    <p class="text-sm text-slate-500">Overview / Dashboard</p>
                    <h2 class="text-2xl md:text-3xl font-bold mt-1">Dashboard Monitoring</h2>
                    <p class="text-sm text-slate-500 mt-1">
                        Pantau kondisi lingkungan kandang secara real-time.
                    </p>
                </div>

                <div class="bg-white border border-slate-200 rounded-xl px-4 py-3 text-sm shadow-sm">
                    <span class="text-slate-400">Device ID</span>
                    <p class="font-semibold mt-1">ESP32-KANDANG-001</p>
                </div>
            </header>

            <!-- Status Banner -->
            <section
                class="bg-white border border-slate-200 rounded-2xl p-5 mb-6 flex flex-col sm:flex-row sm:items-center sm:justify-between gap-3 shadow-sm">
                <div>
                    <p class="text-sm text-slate-500">Device Connection</p>
                    <h3 class="text-lg font-bold mt-1">
                        {{ $isOnline ? 'Perangkat Terhubung' : 'Perangkat Tidak Terhubung' }}
                    </h3>
                    <p class="text-sm text-slate-500 mt-1">
                        Terakhir menerima data:
                        {{ $latestTelemetry?->created_at?->format('d M Y, H:i:s') ?? 'Belum ada data' }}
                    </p>
                </div>

                <span
                    class="inline-flex items-center gap-2 px-4 py-2 rounded-full text-sm font-semibold w-fit
                {{ $isOnline ? 'bg-emerald-50 text-emerald-700' : 'bg-red-50 text-red-700' }}">
                    <span class="w-2 h-2 rounded-full {{ $isOnline ? 'bg-emerald-500' : 'bg-red-500' }}"></span>
                    {{ $isOnline ? 'ONLINE' : 'OFFLINE' }}
                </span>
            </section>

            <!-- Summary Cards -->
            <section id="monitoring" class="grid grid-cols-1 sm:grid-cols-2 xl:grid-cols-4 gap-5 mb-8">

                <div class="bg-white border border-slate-200 rounded-2xl p-5 shadow-sm">
                    <div class="flex justify-between items-center">
                        <p class="text-sm text-slate-500">Temperature</p>
                        <span
                            class="w-10 h-10 rounded-xl bg-orange-50 text-orange-500 flex items-center justify-center text-xl">♨</span>
                    </div>
                    <h3 class="text-3xl font-bold mt-4">
                        {{ $latestTelemetry ? number_format($latestTelemetry->temperature, 1) : '--' }}
                        <span class="text-base text-slate-400">°C</span>
                    </h3>
                    <p class="text-xs text-slate-400 mt-2">Suhu lingkungan kandang</p>
                </div>

                <div class="bg-white border border-slate-200 rounded-2xl p-5 shadow-sm">
                    <div class="flex justify-between items-center">
                        <p class="text-sm text-slate-500">Humidity</p>
                        <span
                            class="w-10 h-10 rounded-xl bg-blue-50 text-blue-500 flex items-center justify-center text-xl">◉</span>
                    </div>
                    <h3 class="text-3xl font-bold mt-4">
                        {{ $latestTelemetry ? number_format($latestTelemetry->humidity, 1) : '--' }}
                        <span class="text-base text-slate-400">%</span>
                    </h3>
                    <p class="text-xs text-slate-400 mt-2">Kelembapan lingkungan</p>
                </div>

                <div class="bg-white border border-slate-200 rounded-2xl p-5 shadow-sm">
                    <div class="flex justify-between items-center">
                        <p class="text-sm text-slate-500">Device Status</p>
                        <span
                            class="w-10 h-10 rounded-xl bg-emerald-50 text-emerald-600 flex items-center justify-center text-xl">⌁</span>
                    </div>
                    <h3 class="text-2xl font-bold mt-4 {{ $isOnline ? 'text-emerald-600' : 'text-red-500' }}">
                        {{ $isOnline ? 'Online' : 'Offline' }}
                    </h3>
                    <p class="text-xs text-slate-400 mt-2">Status berdasarkan data terakhir</p>
                </div>

                <div class="bg-white border border-slate-200 rounded-2xl p-5 shadow-sm">
                    <div class="flex justify-between items-center">
                        <p class="text-sm text-slate-500">Total Records</p>
                        <span
                            class="w-10 h-10 rounded-xl bg-violet-50 text-violet-500 flex items-center justify-center text-xl">▤</span>
                    </div>
                    <h3 class="text-3xl font-bold mt-4">
                        {{ number_format($totalTelemetry) }}
                    </h3>
                    <p class="text-xs text-slate-400 mt-2">Data tersimpan di database</p>
                </div>

            </section>

            <!-- Chart -->
            <section class="bg-white border border-slate-200 rounded-2xl p-5 md:p-6 shadow-sm mb-8">
                <div class="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-2 mb-6">
                    <div>
                        <h3 class="font-bold text-lg">Sensor Analytics</h3>
                        <p class="text-sm text-slate-500">Riwayat perubahan suhu dan kelembapan</p>
                    </div>
                    <span class="text-xs text-slate-400">20 data terakhir</span>
                </div>

                <div class="h-72">
                    <canvas id="telemetryChart"></canvas>
                </div>
            </section>

            <!-- History Table -->
            <section id="history" class="bg-white border border-slate-200 rounded-2xl shadow-sm overflow-hidden">
                <div class="p-5 md:p-6 border-b border-slate-100">
                    <h3 class="font-bold text-lg">Recent Telemetry</h3>
                    <p class="text-sm text-slate-500 mt-1">Data sensor yang diterima sistem</p>
                </div>

                <div class="overflow-x-auto">
                    <table class="w-full text-sm text-left">
                        <thead class="bg-slate-50 text-slate-500">
                            <tr>
                                <th class="px-6 py-4 font-medium">No.</th>
                                <th class="px-6 py-4 font-medium">Device</th>
                                <th class="px-6 py-4 font-medium">Temperature</th>
                                <th class="px-6 py-4 font-medium">Humidity</th>
                                <th class="px-6 py-4 font-medium">Timestamp</th>
                            </tr>
                        </thead>

                        <tbody class="divide-y divide-slate-100">
                            @forelse($history as $index => $item)
                                <tr class="hover:bg-slate-50">
                                    <td class="px-6 py-4 text-slate-400">
                                        {{ $history->firstItem() + $index }}
                                    </td>
                                    <td class="px-6 py-4 font-medium">
                                        {{ $item->device_id }}
                                    </td>
                                    <td class="px-6 py-4">
                                        {{ number_format($item->temperature, 1) }} °C
                                    </td>
                                    <td class="px-6 py-4">
                                        {{ number_format($item->humidity, 1) }} %
                                    </td>
                                    <td class="px-6 py-4 text-slate-500">
                                        {{ $item->created_at->format('d M Y, H:i:s') }}
                                    </td>
                                </tr>
                            @empty
                                <tr>
                                    <td colspan="5" class="px-6 py-8 text-center text-slate-400">
                                        Belum ada data telemetry.
                                    </td>
                                </tr>
                            @endforelse
                        </tbody>

                    </table>


                    <div class="px-6 py-4 border-t border-slate-100">
                        {{ $history->links() }}
                    </div>

                </div>
            </section>

            <footer class="text-center text-xs text-slate-400 mt-8">
                Smart Coop Monitoring System · Laravel & ESP32
            </footer>

        </main>
    </div>

    <script>
        const labels = @json($chartHistory->map(fn($item) => $item->created_at->format('H:i:s')));

        const temperatures = @json($chartHistory->pluck('temperature'));
        const humidities = @json($chartHistory->pluck('humidity'));

        new Chart(document.getElementById('telemetryChart'), {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                        label: 'Temperature (°C)',
                        data: temperatures,
                        borderColor: '#f97316',
                        backgroundColor: 'rgba(249, 115, 22, 0.08)',
                        tension: 0.4,
                        fill: true,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Humidity (%)',
                        data: humidities,
                        borderColor: '#0ea5e9',
                        backgroundColor: 'rgba(14, 165, 233, 0.08)',
                        tension: 0.4,
                        fill: true,
                        yAxisID: 'y1'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: {
                    mode: 'index',
                    intersect: false
                },
                plugins: {
                    legend: {
                        position: 'bottom'
                    }
                },
                scales: {
                    y: {
                        type: 'linear',
                        position: 'left',
                        title: {
                            display: true,
                            text: 'Temperature (°C)'
                        }
                    },
                    y1: {
                        type: 'linear',
                        position: 'right',
                        title: {
                            display: true,
                            text: 'Humidity (%)'
                        },
                        grid: {
                            drawOnChartArea: false
                        },
                        min: 0,
                        max: 100
                    }
                }
            }
        });
    </script>

</body>

</html>
