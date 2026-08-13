# Jevaing Documentation

Version: `0.0.11`
Status: early prototype, not production ready

## 1. Overview

Jevaing is an experimental C++17 game engine built with CMake. Its current focus is to provide a small but usable foundation for standalone games, engine samples, editor tooling, 2D/3D rendering, physics, and early platform target preparation.

Jevaing 0.0.11 adds the first editor/tooling pass and keeps the 0.0.10 physics foundation in place. The engine can now build an external game executable through a generated project template, and the editor can open, edit, preview, play, pause, step, save, and build a project.

The project intentionally keeps engine APIs neutral. Client code uses Jevaing concepts such as `Scene`, components, `Graphics2D`, `Graphics3D`, `PhysicsWorld2D`, `PhysicsWorld3D`, and `Input`. Platform or backend details such as Win32, DirectX, XInput, Box2D, Jolt, ImGui, UWP, or GDK are not exposed through the public API.

## 2. Repository Layout

```text
Jevaing/
|-- Api/                 Public engine headers for game code.
|-- Engine/              Runtime implementation, renderer, assets, physics, build tooling.
|-- Editor/              JevaingEditor executable source.
|-- Sandbox/             JevaingSandbox test/sample host.
|-- Samples/             External consumer samples.
|-- geometry/            Local sample assets.
|-- Library/             Runtime library area.
|-- Documentation/       Existing documentation area.
|-- documents/           Bilingual user-facing documentation generated for 0.0.11.
|-- CMakeLists.txt       Root CMake build.
|-- README.md            Main project overview.
|-- THIRD_PARTY.md       Third-party dependency notes.
`-- LICENSE
```

## 3. Build Requirements

Recommended Windows desktop setup:

- Windows 10 or newer.
- CMake 3.20 or newer.
- Visual Studio / MSVC toolchain.
- Git, because dependencies are fetched by CMake when not available locally.

Core dependencies fetched or linked by the build include:

- Assimp for model loading.
- Box2D 3.1.1 for 2D physics when enabled.
- Jolt 5.6.0 for 3D physics when enabled.
- Dear ImGui 1.92.1 docking for `JevaingEditor` when editor builds are enabled.

## 4. Build From Source

Configure and build the default Debug configuration:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Main outputs:

```text
bin/Debug/JevaingSandbox.exe
bin/Debug/JevaingEditor.exe
build/Samples/XboxSmokeTest/bin/Debug/XboxSmokeTest.exe
```

Useful CMake options:

```powershell
cmake -S . -B build-runtime -DJEVAING_BUILD_EDITOR=OFF
cmake --build build-runtime --config Debug

cmake -S . -B build-nophysics `
  -DJEVAING_ENABLE_PHYSICS_2D=OFF `
  -DJEVAING_ENABLE_PHYSICS_3D=OFF `
  -DJEVAING_BUILD_EDITOR=OFF `
  -DJEVAING_BUILD_XBOX_SMOKE_TEST=OFF
cmake --build build-nophysics --config Debug
```

## 5. JevaingEditor

Run the editor:

```powershell
.\bin\Debug\JevaingEditor.exe
```

Print editor capability information without opening the UI:

```powershell
.\bin\Debug\JevaingEditor.exe --editor-info
```

Current editor panels:

- `Project Browser`: create or open projects.
- `Hierarchy`: create/select entities and view parent-child relationships.
- `Scene`: editor view for selection, grid feedback, and mouse-based transform editing.
- `Game`: preview of the game camera framing.
- `Inspector`: edit names, transforms, and core components.
- `Project`: Unity-style project browser for `Assets`, `Scenes`, and `Source`.
- `Console`: displays engine/editor log messages.
- `Build Settings`: choose build target/configuration and build the Windows Desktop executable.

Top menu:

- `File`: save, close project, stop play mode, exit.
- `Edit`, `GameObject`, `Assets`, `Build`, `Help`: reserved menu sections for future tooling.
- `Windows`: show or hide editor panels.

Play controls:

- `Play`: creates a runtime snapshot of the edit scene.
- `Pause`: stops simulation updates while staying in Play Mode.
- `Resume`: resumes simulation after pause.
- `Step`: advances one simulation frame while paused.
- `Stop`: discards the runtime scene and restores the edit scene snapshot.

The editor warns about unsaved scene changes before exiting, closing a project, or loading another project/scene.

## 6. Scene And Game Views

`Scene` and `Game` are separate editor windows.

The `Scene` view is an editor preview. It draws a grid and simple entity bounds, allows selecting entities with the mouse, and lets the user drag selected entities on the X/Y plane while not in Play Mode.

The `Game` view is a camera preview. It draws a 16:9 framed preview using the scene camera as the offset reference. If a primary camera exists, it is preferred; otherwise the first camera component is used. This view is currently an MVP editor-rendered preview and is not yet the final embedded D3D11 runtime render target.

## 7. Project Workflow

A generated project contains:

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

The project CMake file expects `JEVAING_ENGINE_ROOT` at configure time and does not store personal absolute paths. It compiles all `Source/*.cpp` and `Source/*.c` files through `GAME_SOURCES`, so a project can mix C++ gameplay entry code with C helper modules.

The editor Project panel supports creating:

- C++ script files.
- C source/header files.
- Header files.
- Folders.

C and C++ files are created under `Source` so they participate in the generated build.

## 8. Public Game API

Game code includes:

```cpp
#include <Jevaing/Jevaing.h>
```

Primary public concepts:

- `Game` and `GameConfig`.
- `Run(...)`.
- `Project` and `ProjectConfig`.
- `Scene` and `SceneEntity`.
- `EntityId`.
- `TransformComponent`.
- `CameraComponent`.
- `MeshRendererComponent`.
- `SpriteRenderer2DComponent`.
- 2D/3D rigid bodies and colliders.
- `Graphics2D` and `Graphics3D`.
- keyboard, mouse, gamepad, and input maps.
- `PhysicsWorld2D` and `PhysicsWorld3D`.

Public API policy:

- No ImGui types.
- No Win32 handles.
- No DirectX handles.
- No XInput handles.
- No Box2D types.
- No Jolt types.
- No UWP/GDK types.

## 9. Rendering

Current renderer work includes:

- DirectX 11 desktop renderer.
- Null renderer for headless/core validation.
- 2D drawing and 3D drawing APIs.
- Model loading through Assimp.
- Texture loading and material-oriented tests.
- Visual smoke tests for triangles, textured sprites, cube rendering, lighting, models, and mixed 2D/3D content.

The editor preview currently draws simplified ImGui representations. It is useful for editing and camera framing, but it is not yet a final offscreen runtime renderer embedded inside the editor.

## 10. Physics

Jevaing 0.0.10/0.0.11 keeps physics behind neutral public APIs.

2D physics:

- Public API: `PhysicsWorld2D`, `RigidBody2DComponent`, `BoxCollider2DComponent`, `CircleCollider2DComponent`, 2D raycasts/events.
- Backend: Box2D 3.1.1.

3D physics:

- Public API: `PhysicsWorld3D`, `RigidBody3DComponent`, `BoxCollider3DComponent`, `SphereCollider3DComponent`, `CapsuleCollider3DComponent`, 3D raycasts/events.
- Backend: Jolt 5.6.0.

Scene code works with Jevaing components and neutral physics worlds. Backend translation belongs to the backend implementation.

## 11. Input And Gamepad

Jevaing exposes neutral input APIs for keyboard, mouse, and gamepad.

Gamepad example:

```cpp
if (Jevaing::Input::IsGamepadConnected(0))
{
    Jevaing::GamepadState state = Jevaing::Input::GetGamepadState(0);
    if (Jevaing::Input::IsGamepadButtonPressed(0, Jevaing::GamepadButton::A))
    {
        // Handle A button.
    }
}
```

Windows Desktop currently uses XInput behind the public API.

## 12. Build Targets

Current target status:

- Windows Desktop x64: functional build target.
- Xbox Dev Mode / UWP x64: experimental environment-gated smoke target.
- Xbox One / GDK x64: future/preparation target, unavailable without an authorized GDK environment.
- Xbox Series X|S / GDK x64: future/preparation target, unavailable without an authorized GDK environment.

The editor can build Windows Desktop x64 projects in this pass. Other targets are listed for visibility and environment detection.

Xbox is not verified until a package starts on real Xbox hardware and the user confirms it.

## 13. Xbox Smoke Test

`Samples/XboxSmokeTest` is the first minimal Xbox-oriented smoke target:

- app startup
- DirectX runtime path where supported
- triangle/cube rendering
- gamepad through the neutral Jevaing input API

Desktop smoke:

```powershell
cmake --build build --config Debug --target XboxSmokeTest
.\build\Samples\XboxSmokeTest\bin\Debug\XboxSmokeTest.exe --frames 120
```

Experimental UWP configure shape:

```powershell
cmake -S Samples\XboxSmokeTest -B build-xbox-smoke-uwp `
  -DCMAKE_SYSTEM_NAME=WindowsStore `
  -DCMAKE_SYSTEM_VERSION=10.0 `
  -DCMAKE_GENERATOR_PLATFORM=x64 `
  -DJEVAING_ENGINE_ROOT=<path-to-jevaing>
cmake --build build-xbox-smoke-uwp --config Debug
```

No certificate, private key, or private Microsoft SDK files are committed to the repository.

## 14. Samples

Minimal3D external sample:

```powershell
cmake -S Samples\Minimal3D -B build-minimal3d
cmake --build build-minimal3d --config Debug
.\build-minimal3d\bin\Debug\Minimal3D.exe --frames 120
```

Physics3D external sample:

```powershell
cmake -S Samples\Physics3D -B build-physics3d
cmake --build build-physics3d --config Debug
.\build-physics3d\bin\Debug\Physics3D.exe --frames 120
```

These samples are important because they consume Jevaing as an external engine dependency rather than relying on internal sandbox flags.

## 15. Command-Line Validation

Core:

```powershell
.\bin\Debug\JevaingSandbox.exe --self-test
.\bin\Debug\JevaingSandbox.exe --runtime-test
.\bin\Debug\JevaingSandbox.exe --scene-serialization-test
.\bin\Debug\JevaingSandbox.exe --hierarchy-test
```

Rendering/assets:

```powershell
.\bin\Debug\JevaingSandbox.exe --renderer-info
.\bin\Debug\JevaingSandbox.exe --graphics-test
.\bin\Debug\JevaingSandbox.exe --graphics-test-3d
.\bin\Debug\JevaingSandbox.exe --texture-test
.\bin\Debug\JevaingSandbox.exe --material-test
.\bin\Debug\JevaingSandbox.exe --lighting-test
.\bin\Debug\JevaingSandbox.exe --multi-model-test
.\bin\Debug\JevaingSandbox.exe --asset-cache-test
.\bin\Debug\JevaingSandbox.exe --asset-error-test
```

Physics:

```powershell
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

Editor/build/platform:

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

## 16. Manual Editor Validation Checklist

1. Launch `JevaingEditor.exe`.
2. Create a new project.
3. Open the generated `Scenes/main.scene`.
4. Select entities in `Hierarchy`.
5. Select and drag entities in `Scene`.
6. Verify the camera-framed preview in `Game`.
7. Add/remove components in `Inspector`.
8. Create C and C++ files from the `Project` panel.
9. Press Play.
10. Press Pause.
11. Press Step.
12. Press Resume.
13. Press Stop and verify edit state is restored.
14. Modify the scene and try closing/loading another scene to verify the unsaved-changes modal.
15. Save.
16. Build Windows Desktop x64 from `Build Settings`.
17. Run the generated executable.

## 17. Known Limitations

- The editor is still an MVP and not a production editor.
- `Scene` and `Game` are simplified editor previews, not final embedded runtime render targets.
- Mouse editing currently supports basic X/Y dragging, not a full transform gizmo.
- The `Project` panel creates code files but does not provide an integrated code editor.
- Xbox Dev Mode/UWP is not verified in hardware.
- GDK targets require authorized Microsoft tooling and are not implemented as full shipping targets.
- Visual validation still requires a person.

## 18. Troubleshooting

If the editor fails to open:

```powershell
cmake --build build --config Debug
.\bin\Debug\JevaingEditor.exe --editor-info
```

If physics commands report unavailable backends, confirm physics was not disabled in the active build directory.

If an external project cannot configure, pass `JEVAING_ENGINE_ROOT`:

```powershell
cmake -S <project> -B <project>\.jevaing-build\Windows -DJEVAING_ENGINE_ROOT=<path-to-jevaing>
```

If generated C/C++ files are not compiled, ensure they are under the project's `Source` directory.

## 19. Development Notes

Keep public API changes backend-neutral. Backend-specific implementation belongs in engine internals. Editor-only dependencies should stay linked only to `JevaingEditor`, not to external game executables.

When adding platform support, prefer small smoke tests before full engine ports. Xbox should remain marked as not verified until real hardware validation is completed by the user.
