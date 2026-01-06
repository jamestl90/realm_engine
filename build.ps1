param(
    [switch]$Clean
)

$modeFile = ".build_mode"
$config = if (Test-Path $modeFile) { Get-Content $modeFile } else { "Debug" }

function Remove-BuildFolder {
    if (Test-Path "build") {
        Write-Host "Removing build folder..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force "build"
    }
}

function Configure-CMake {
    if (-not (Test-Path "build")) { 
        New-Item -ItemType Directory -Path "build" | Out-Null
    }
    
    Write-Host "Configuring CMake..." -ForegroundColor Cyan
    Push-Location "build"
    $configOutput = cmake .. 2>&1
    $configExitCode = $LASTEXITCODE
    Pop-Location
    
    if ($configExitCode -ne 0) {
        Write-Host $configOutput -ForegroundColor Red
        return $false
    }
    return $true
}

function Run-Build {
    param(
        [bool]$AllowRetry = $true
    )
    
    # Ensure build folder and CMake config exist
    if (-not (Test-Path "build\CMakeCache.txt")) {
        if (-not (Configure-CMake)) {
            return $false
        }
    }
    
    # Run the build and capture output
    Write-Host "Building..." -ForegroundColor Cyan
    $buildOutput = cmake --build build --config $config 2>&1 | Out-String
    $buildExitCode = $LASTEXITCODE
    
    if ($buildExitCode -ne 0) {
        # Check if it's a cache-related error
        $isCacheError = $buildOutput -match "(?i)cache|CMAKE_PROJECT_NAME|CMakeCache"
        
        if ($isCacheError -and $AllowRetry) {
            Write-Host "Cache error detected. Wiping build folder and retrying..." -ForegroundColor Yellow
            Remove-BuildFolder
            
            if (-not (Configure-CMake)) {
                Write-Host $buildOutput -ForegroundColor Red
                return $false
            }
            
            # Retry build without allowing another retry
            return Run-Build -AllowRetry $false
        }
        
        Write-Host $buildOutput -ForegroundColor Red
        return $false
    }
    
    Write-Host $buildOutput
    return $true
}

# Main script execution
Write-Host "--- Build Starting: Configuration [$config] ---" -ForegroundColor Cyan

# Handle -Clean switch
if ($Clean) {
    Remove-BuildFolder
    Write-Host "Build folder cleaned." -ForegroundColor Green
    exit 0
}

# Run the build
$success = Run-Build

if (-not $success) {
    Write-Host "--- BUILD FAILED ---" -ForegroundColor Red
    exit 1 
} else {
    Write-Host "--- BUILD SUCCESS ---" -ForegroundColor Green
    exit 0
}
