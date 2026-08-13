param(
    [ValidateSet("debug-no-tests", "release-no-tests", "debug-with-tests")]
    [string]$Preset = "debug-no-tests",
    [switch]$Clean
)

$buildDir = "out\build\$Preset"
$vsDevCmd = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

function Invoke-CMake {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    if (-not (Test-Path $vsDevCmd)) {
        Write-Host "Visual Studio build environment was not found at $vsDevCmd" -ForegroundColor Red
        $script:CMakeExitCode = 1
        return @()
    }

    $cmakeArguments = $Arguments -join " "
    $command = "call `"$vsDevCmd`" -arch=x64 -host_arch=x64 >nul && cmake $cmakeArguments"
    $output = cmd /d /s /c $command 2>&1
    $script:CMakeExitCode = $LASTEXITCODE
    return $output
}

function Remove-BuildFolder {
    if (Test-Path $buildDir) {
        Write-Host "Removing $buildDir..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force $buildDir
    }
}

function Configure-CMake {
    Write-Host "Configuring CMake..." -ForegroundColor Cyan
    $configOutput = Invoke-CMake @("--preset", $Preset)
    $configExitCode = $script:CMakeExitCode
    
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
    
    # Ensure build folder and CMake config exist.
    if (-not (Test-Path "$buildDir\CMakeCache.txt")) {
        if (-not (Configure-CMake)) {
            return $false
        }
    }
    
    # Run the build and capture output
    Write-Host "Building..." -ForegroundColor Cyan
    $buildOutput = Invoke-CMake @("--build", "--preset", $Preset) | Out-String
    $buildExitCode = $script:CMakeExitCode
    
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
Write-Host "--- Build Starting: Preset [$Preset] ---" -ForegroundColor Cyan

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
