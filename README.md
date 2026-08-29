# Realm Engine

2D / SDL3 engine for procedural map generation

## Prerequisites

- Windows 10 or later
- Git
- CMake 3.20 or later
- Ninja
- Visual Studio 2022 Build Tools (or a newer Visual Studio installation) with
  the **Desktop development with C++** workload
- Internet access on the first configure so CMake can fetch the pinned SDL3,
  SDL3_ttf, and RapidJSON sources

`dxc` and `spirv-cross` are only needed when changing shader sources. Generated
runtime shaders are committed to the repository.

## Build

```powershell
git clone <repository-url> realm_engine
cd realm_engine
.\scripts\build.ps1 -Preset debug-no-tests
```

The executable and its runtime DLLs/assets are written to
`out\build\debug-no-tests`.

Other supported presets are:

```powershell
.\scripts\build.ps1 -Preset release-no-tests
.\scripts\build.ps1 -Preset debug-with-tests
ctest --preset debug-with-tests
```

Clean one preset's build tree with:

```powershell
.\scripts\build.ps1 -Preset debug-no-tests -Clean
```

## Shader development

After editing a file under `shaders/`, install DirectX Shader Compiler with
SPIR-V support and SPIRV-Cross, put `dxc` and `spirv-cross` on `PATH`, then run:

```powershell
.\shaders\compile_shaders.ps1 -Force
```

Commit the updated files under `assets/Shaders` together with the HLSL change.

## Screenshots

![Procedural map generation screenshot](screenshots/Screenshot%202026-08-16%20194726.png)

![Procedural map generation demo](screenshots/demo1.gif)
