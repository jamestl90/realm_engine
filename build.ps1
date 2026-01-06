$modeFile = ".build_mode"
$config = if (Test-Path $modeFile) { Get-Content $modeFile } else { "Debug" }

Write-Host "--- Build Starting: Configuration [$config] ---" -ForegroundColor Cyan

if (-not (Test-Path "build")) { New-Item -ItemType Directory -Path "build" }
if (-not (Test-Path "build\CMakeCache.txt")) {
    Push-Location "build"
    cmake ..
    Pop-Location
}

# Run the build
cmake --build build --config $config

# CRITICAL: Capture the exit code so Aider knows if it failed
$buildExitCode = $LASTEXITCODE

if ($buildExitCode -ne 0) {
    Write-Host "--- BUILD FAILED ---" -ForegroundColor Red
    exit 1 
} else {
    Write-Host "--- BUILD SUCCESS ---" -ForegroundColor Green
    exit 0
}