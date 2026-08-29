param(
    [ValidateSet("debug-no-tests", "release-no-tests", "debug-with-tests")]
    [string]$Preset = "debug-no-tests",
    [switch]$Clean
)

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repositoryRoot "out\build\$Preset"
$shaderBuildScript = Join-Path $repositoryRoot "shaders\compile_shaders.ps1"

function Find-VsDevCmd {
    $vswhereCandidates = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    foreach ($vswhere in $vswhereCandidates) {
        $installationPath = & $vswhere `
            -latest `
            -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath

        if ($LASTEXITCODE -eq 0 -and $installationPath) {
            $candidate = Join-Path ($installationPath | Select-Object -First 1) "Common7\Tools\VsDevCmd.bat"
            if (Test-Path -LiteralPath $candidate) {
                return $candidate
            }
        }
    }

    # Fallbacks for machines where vswhere is unavailable.
    $fallbacks = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
    )

    return $fallbacks | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
}

$vsDevCmd = Find-VsDevCmd

function Invoke-CMake {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    if (-not $vsDevCmd) {
        Write-Host "A Visual Studio installation with the C++ build tools was not found." -ForegroundColor Red
        Write-Host "Install the 'Desktop development with C++' workload and retry." -ForegroundColor Red
        $script:CMakeExitCode = 1
        return @()
    }

    $cmakeArguments = $Arguments -join " "
    $command = "call `"$vsDevCmd`" -arch=x64 -host_arch=x64 >nul && cd /d `"$repositoryRoot`" && cmake $cmakeArguments"
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

function Build-Shaders {
    Write-Host "Checking shaders..." -ForegroundColor Cyan
    & $shaderBuildScript
    if ($LASTEXITCODE -ne 0) {
        return $false
    }
    return $true
}

function Run-Build {
    param(
        [bool]$AllowRetry = $true
    )

    # Reapply the selected preset so changed cache variables take effect in existing build trees.
    if (-not (Configure-CMake)) {
        return $false
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

# Verify that the committed runtime shader assets are present. Developers can
# regenerate them explicitly with shaders/compile_shaders.ps1 -Force.
$success = Build-Shaders
if ($success) {
    $success = Run-Build
}

if (-not $success) {
    Write-Host "--- BUILD FAILED ---" -ForegroundColor Red
    exit 1 
} else {
    Write-Host "--- BUILD SUCCESS ---" -ForegroundColor Green
    exit 0
}
