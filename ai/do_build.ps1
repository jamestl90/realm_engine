$modeFile = ".build_mode"
$config = if (Test-Path $modeFile) { Get-Content $modeFile } else { "Debug" }

# Updated label to match your on-demand workflow
Write-Host "--- On-Demand Build: Configuration [$config] ---" -ForegroundColor Cyan

# Ensure build directory exists and CMake is configured
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build"
}
if (-not (Test-Path "build\CMakeCache.txt")) {
    Push-Location "build"
    cmake ..
    Pop-Location
}

cmake --build build --config $config
