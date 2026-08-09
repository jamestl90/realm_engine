$sourceFolder = $PSScriptRoot
$repositoryRoot = Split-Path -Parent $sourceFolder
$outputFolder = Join-Path $repositoryRoot "assets\Shaders"

# Make sure output folder exists
if (-not (Test-Path $outputFolder)) {
    New-Item -ItemType Directory -Force -Path $outputFolder
}

# Helper function to compile HLSL to SPIR-V and reflect
function Compile-And-Reflect($filePath, $stage) {
    $fileName = [System.IO.Path]::GetFileNameWithoutExtension($filePath)
    $spvOutput = Join-Path $outputFolder "$fileName.spv"
    $reflectOutput = Join-Path $outputFolder "$fileName.reflect.json"

    # Compile HLSL to SPIR-V
    switch ($stage) {
        "vert" { $target = "vs_6_6" }
        "frag" { $target = "ps_6_6" }
        default { Write-Error "Unknown stage $stage"; return }
    }

    Write-Host "Compiling $filePath -> $spvOutput"
    dxc $filePath -T $target -E main -Fo $spvOutput -spirv

    # Generate reflection JSON
    Write-Host "Reflecting $spvOutput -> $reflectOutput"
    spirv-cross $spvOutput --reflect --output $reflectOutput

}

# Compile vertex shaders
Get-ChildItem -Path $sourceFolder -Filter "*.vert.hlsl" | ForEach-Object {
    Compile-And-Reflect $_.FullName "vert"
}

# Compile fragment shaders
Get-ChildItem -Path $sourceFolder -Filter "*.frag.hlsl" | ForEach-Object {
    Compile-And-Reflect $_.FullName "frag"
}

Write-Host "All runtime shaders compiled and reflected into $outputFolder"
