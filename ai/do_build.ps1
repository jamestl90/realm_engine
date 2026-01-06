$modeFile = ".build_mode"
$config = if (Test-Path $modeFile) { Get-Content $modeFile } else { "Debug" }

# Updated label to match your on-demand workflow
Write-Host "--- On-Demand Build: Configuration [$config] ---" -ForegroundColor Cyan

cmake --build build --config $config
