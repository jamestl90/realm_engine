# Set paths
$sourceFolder = "D:\repos\2D_Game_1\shaders"  
$outputFolder = "D:\repos\2D_Game_1\build\debug\assets\shaders"

# Make sure output folder exists
if (-not (Test-Path $outputFolder)) {
    New-Item -ItemType Directory -Force -Path $outputFolder
}

# Helper function to compile HLSL to SPIR-V and reflect
function Compile-And-Reflect($filePath, $stage) {
    $fileName = [System.IO.Path]::GetFileNameWithoutExtension($filePath)
    $spvOutput = Join-Path $sourceFolder "$fileName.spv"
    $reflectOutput = Join-Path $sourceFolder "$fileName.reflect.json"

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

    # Copy outputs to build folder
    Copy-Item $spvOutput -Destination $outputFolder -Force
    Copy-Item $reflectOutput -Destination $outputFolder -Force
}

# Compile vertex shaders
Get-ChildItem -Path $sourceFolder -Filter "*.vert.hlsl" | ForEach-Object {
    Compile-And-Reflect $_.FullName "vert"
}

# Compile fragment shaders
Get-ChildItem -Path $sourceFolder -Filter "*.frag.hlsl" | ForEach-Object {
    Compile-And-Reflect $_.FullName "frag"
}

Write-Host "All shaders compiled, reflected, and copied to $outputFolder"
