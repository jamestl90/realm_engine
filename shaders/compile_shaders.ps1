[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$sourceFolder = $PSScriptRoot
$repositoryRoot = Split-Path -Parent $sourceFolder
$outputFolder = Join-Path $repositoryRoot "assets\Shaders"

function Assert-CommandAvailable {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Command
    )

    if (-not (Get-Command $Command -ErrorAction SilentlyContinue)) {
        throw "Required shader tool '$Command' was not found on PATH."
    }
}

if (-not (Test-Path -LiteralPath $outputFolder)) {
    New-Item -ItemType Directory -Force -Path $outputFolder | Out-Null
}

function Test-ShaderNeedsBuild($filePath) {
    $fileName = [System.IO.Path]::GetFileNameWithoutExtension($filePath)
    $spvOutput = Join-Path $outputFolder "$fileName.spv"
    $reflectOutput = Join-Path $outputFolder "$fileName.reflect.json"

    # Generated outputs are committed so normal builds do not require shader
    # toolchains. Use -Force after intentionally editing an HLSL source.
    return $Force -or
        -not (Test-Path -LiteralPath $spvOutput) -or
        -not (Test-Path -LiteralPath $reflectOutput)
}

$shaders = @(
    Get-ChildItem -Path $sourceFolder -Filter "*.vert.hlsl" | ForEach-Object {
        [pscustomobject]@{ FilePath = $_.FullName; Stage = "vert" }
    }
    Get-ChildItem -Path $sourceFolder -Filter "*.frag.hlsl" | ForEach-Object {
        [pscustomobject]@{ FilePath = $_.FullName; Stage = "frag" }
    }
)
$staleShaders = @($shaders | Where-Object { Test-ShaderNeedsBuild $_.FilePath })

if ($staleShaders.Count -eq 0) {
    Write-Host "Shaders are up to date."
    exit 0
}

$dxc = Get-Command dxc -ErrorAction SilentlyContinue
$spirvCross = Get-Command spirv-cross -ErrorAction SilentlyContinue
if (-not $dxc) {
    throw "dxc was not found on PATH. Install a DirectX Shader Compiler build with SPIR-V support."
}
if (-not $spirvCross) {
    throw "spirv-cross was not found on PATH. Install SPIRV-Cross and add it to PATH."
}

function Compile-And-Reflect($filePath, $stage) {
    $fileName = [System.IO.Path]::GetFileNameWithoutExtension($filePath)
    $spvOutput = Join-Path $outputFolder "$fileName.spv"
    $reflectOutput = Join-Path $outputFolder "$fileName.reflect.json"
    $temporarySuffix = ".tmp-$([Guid]::NewGuid().ToString('N'))"
    $temporarySpv = "$spvOutput$temporarySuffix"
    $temporaryReflection = "$reflectOutput$temporarySuffix"

    switch ($stage) {
        "vert" { $target = "vs_6_6" }
        "frag" { $target = "ps_6_6" }
        default { throw "Unknown shader stage: $stage" }
    }

    try {
        Write-Host "Compiling $filePath -> $spvOutput"
        & $dxc.Source $filePath -T $target -E main -Fo $temporarySpv -spirv
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $temporarySpv)) {
            throw "dxc failed to compile $filePath. Ensure this dxc build includes SPIR-V support."
        }

        Write-Host "Reflecting $spvOutput -> $reflectOutput"
        & $spirvCross.Source $temporarySpv --reflect --output $temporaryReflection
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $temporaryReflection)) {
            throw "spirv-cross failed to reflect $spvOutput."
        }

        Move-Item -LiteralPath $temporarySpv -Destination $spvOutput -Force
        Move-Item -LiteralPath $temporaryReflection -Destination $reflectOutput -Force
    } finally {
        Remove-Item -LiteralPath $temporarySpv -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $temporaryReflection -Force -ErrorAction SilentlyContinue
    }
}

foreach ($shader in $staleShaders) {
    Compile-And-Reflect $shader.FilePath $shader.Stage
}

Write-Host "$($staleShaders.Count) shader(s) compiled and reflected into $outputFolder"
