# Jevaing

Jevaing is an experimental open-source graphics and game engine written in C++17 and built with CMake.

> **Current version:** `0.0.11`
>
> **Codename:** `TBD`
>
> **Status:** early prototype, not production ready.

## What 0.0.11 Adds

Jevaing 0.0.11 introduces the first usable editor/tooling pass plus build-target foundations:

- `JevaingEditor.exe` as a separate executable.
- Dear ImGui docking UI for Project Browser, Hierarchy, Scene, Game, Inspector, Project, Console and Build Settings.
- Project creation from a portable C/C++ CMake template.
- Scene save/load through the existing runtime `Scene` format.
- Edit Mode / Play Mode separation with snapshot restore.
- Windows Desktop x64 build target that creates an external game executable.
- Neutral public gamepad API backed by XInput on Windows.
- Experimental Xbox Dev Mode / UWP target entry and `Samples/XboxSmokeTest`.
- Xbox GDK target detection/preparation without private SDK code.

0.0.10 physics remains in place: Box2D 3.1.1 and Jolt 5.6.0 are real private backends behind neutral Jevaing APIs.

## Editor

Build and open:

```powershell
cmake -S . -B build
cmake --build build --config Debug
.\bin\Debug\JevaingEditor.exe
```

Headless editor info:

```powershell
.\bin\Debug\JevaingEditor.exe --editor-info
```

The editor currently provides:

- Project Browser: create/open project.
- Hierarchy: select, create, delete and parent/child display.
- Inspector: edit name/transform and add/remove core components.
- Scene View: MVP editor-rendered grid/bounds preview.
- Project/Assets: scans assets/scenes.
- Console: receives `Logger` events through an internal sink while stdout/stderr still work.
- Play/Pause/Step/Stop: runs a runtime scene snapshot and restores edit state on Stop.
- Windows menu: show/hide editor panels from the top menu bar.
- Unsaved scene warning before closing the editor, closing a project or loading another project/scene.
- Build Settings: Windows Desktop x64 build and target availability display.

Known editor limitation: Scene/Game are not yet final D3D11 offscreen runtime render targets. They are MVP editor previews for selection, camera framing and spatial feedback.

## Project Template

New projects contain:

```text
MyGame/
|-- jevaing.project
|-- Assets/
|   |-- Models/
|   `-- Textures/
|-- Scenes/
|   `-- main.scene
|-- Source/
|   |-- Game.cpp
|   |-- GameCode.c
|   `-- GameCode.h
`-- CMakeLists.txt
```

The generated `CMakeLists.txt` is portable and expects `JEVAING_ENGINE_ROOT` at configure/build time. It does not store personal absolute paths. It compiles `Source/*.cpp` and `Source/*.c`, so projects can mix C++ gameplay entry code with C helper modules.

## Build Targets

Current targets:

- Windows Desktop x64: functional CMake/MSVC build target.
- Xbox Dev Mode / UWP x64: experimental testing target, environment-gated.
- Xbox One / GDK x64: future target, unavailable unless an authorized GDK environment is detected.
- Xbox Series X|S / GDK x64: future target, unavailable unless an authorized GDK environment is detected.

Xbox status is intentionally conservative:

- Xbox Dev Mode/UWP is a testing route, not commercial Game OS/GDK support.
- UWP games are no longer accepted in the Xbox Store per current Microsoft documentation.
- Xbox GDK is a separate authorized route and no private SDK headers/libraries are committed.
- Xbox is **not VERIFIED** until the package starts on real hardware and the user confirms it.

## Xbox Smoke Test

`Samples/XboxSmokeTest` is the first minimal smoke target:

- app startup
- DirectX runtime path where supported
- triangle/cube draw
- gamepad through neutral `Jevaing::Input`

Desktop smoke build from root:

```powershell
cmake --build build --config Debug --target XboxSmokeTest
.\build\Samples\XboxSmokeTest\bin\Debug\XboxSmokeTest.exe --frames 120
```

Experimental UWP configure shape, pending local SDK/package validation:

```powershell
cmake -S Samples\XboxSmokeTest -B build-xbox-smoke-uwp `
  -DCMAKE_SYSTEM_NAME=WindowsStore `
  -DCMAKE_SYSTEM_VERSION=10.0 `
  -DCMAKE_GENERATOR_PLATFORM=x64 `
  -DJEVAING_ENGINE_ROOT=<path-to-jevaing>
cmake --build build-xbox-smoke-uwp --config Debug
```

No certificate/private key is stored in the repo.

## Public API

Client games still use:

```cpp
#include <Jevaing/Jevaing.h>
```

Public runtime APIs remain backend-neutral:

- `Project`
- `Scene`
- `EntityId`
- components
- `Graphics2D` / `Graphics3D`
- keyboard, mouse, gamepad and `InputMap`
- `PhysicsWorld2D` / `PhysicsWorld3D`

The public API does not expose ImGui, Win32, DirectX, XInput, Box2D, Jolt, UWP or GDK handles.

## Gamepad

Neutral API:

```cpp
Jevaing::Input::IsGamepadConnected(0);
Jevaing::Input::IsGamepadButtonPressed(0, Jevaing::GamepadButton::A);
Jevaing::GamepadState state = Jevaing::Input::GetGamepadState(0);
```

Windows Desktop currently uses XInput 1.4 behind this API. XboxSmokeTest uses the same public API.

## External Samples

Minimal3D:

```powershell
cmake -S Samples\Minimal3D -B build-minimal3d
cmake --build build-minimal3d --config Debug
.\build-minimal3d\bin\Debug\Minimal3D.exe --frames 120
```

Physics3D:

```powershell
cmake -S Samples\Physics3D -B build-physics3d
cmake --build build-physics3d --config Debug
.\build-physics3d\bin\Debug\Physics3D.exe --frames 120
```

## Command-line Tests

Core and physics:

```powershell
.\bin\Debug\JevaingSandbox.exe --self-test
.\bin\Debug\JevaingSandbox.exe --runtime-test
.\bin\Debug\JevaingSandbox.exe --scene-serialization-test
.\bin\Debug\JevaingSandbox.exe --hierarchy-test
.\bin\Debug\JevaingSandbox.exe --physics-info
.\bin\Debug\JevaingSandbox.exe --physics-fixed-step-test
.\bin\Debug\JevaingSandbox.exe --physics-3d-test
.\bin\Debug\JevaingSandbox.exe --physics-3d-stack-test
.\bin\Debug\JevaingSandbox.exe --physics-3d-sphere-test
.\bin\Debug\JevaingSandbox.exe --physics-3d-trigger-test
.\bin\Debug\JevaingSandbox.exe --physics-3d-raycast-test
.\bin\Debug\JevaingSandbox.exe --physics-2d-test
.\bin\Debug\JevaingSandbox.exe --physics-2d-circle-test
.\bin\Debug\JevaingSandbox.exe --physics-2d-trigger-test
.\bin\Debug\JevaingSandbox.exe --physics-2d-raycast-test
.\bin\Debug\JevaingSandbox.exe --physics-scene-serialization-test
.\bin\Debug\JevaingSandbox.exe --physics-destroy-test
.\bin\Debug\JevaingSandbox.exe --physics-hierarchy-test
```

0.0.11:

```powershell
.\bin\Debug\JevaingSandbox.exe --build-target-info
.\bin\Debug\JevaingSandbox.exe --gamepad-test
.\bin\Debug\JevaingSandbox.exe --project-template-test
.\bin\Debug\JevaingSandbox.exe --editor-scene-roundtrip-test
.\bin\Debug\JevaingSandbox.exe --playmode-restore-test
.\bin\Debug\JevaingSandbox.exe --windows-build-test
.\bin\Debug\JevaingSandbox.exe --xbox-build-environment-test
.\bin\Debug\JevaingEditor.exe --editor-info
```

Visual/editor tests still require a person. A passing exit code only means the runtime initialized, ran and shut down cleanly.

## Build Options

```powershell
cmake -S . -B build-runtime -DJEVAING_BUILD_EDITOR=OFF
cmake --build build-runtime --config Debug

cmake -S . -B build-nophysics `
  -DJEVAING_ENABLE_PHYSICS_2D=OFF `
  -DJEVAING_ENABLE_PHYSICS_3D=OFF
cmake --build build-nophysics --config Debug
```

`JEVAING_BUILD_EDITOR=OFF` avoids fetching/compiling Dear ImGui.

## Validation Checklist

Automated:

```powershell
git pull origin main
cmake -S . -B build
cmake --build build --config Debug --clean-first
.\bin\Debug\JevaingSandbox.exe --self-test
.\bin\Debug\JevaingSandbox.exe --physics-info
.\bin\Debug\JevaingSandbox.exe --project-template-test
.\bin\Debug\JevaingSandbox.exe --playmode-restore-test
.\bin\Debug\JevaingSandbox.exe --windows-build-test
.\bin\Debug\JevaingSandbox.exe --xbox-build-environment-test
```

Manual editor validation:

1. Open `JevaingEditor.exe`.
2. Create a project.
3. Open `Scenes/main.scene`.
4. Create/edit entities.
5. Add `RigidBody3D` and `BoxCollider3D`.
6. Press Play and verify physics moves the cube.
7. Press Stop and verify edit transforms restore.
8. Save.
9. Build Windows Desktop x64.
10. Run the generated game executable.

Manual Xbox validation:

1. Configure the Xbox Dev Mode/UWP smoke target with the local SDK.
2. Build/package locally.
3. Install via official Xbox Developer Mode tooling.
4. Confirm the package starts on the console.

Until step 4 is confirmed by the user, Xbox remains **PENDING HARDWARE VALIDATION**.

## Current Limitations

- Scene View is MVP editor drawing, not final embedded runtime render target.
- No prefab system, scripting, asset database, thumbnails, gizmos, animation editor or material editor.
- Xbox Dev Mode/UWP packaging is environment-gated and not hardware-verified.
- Xbox GDK targets are detection/preparation only.
- Dynamic physics bodies under parent entities remain rejected.
- Vulkan, Metal, Linux and macOS backends are placeholders.

## License

Jevaing is licensed under the MIT License.
