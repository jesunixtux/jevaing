# Jevaing

Jevaing is an experimental open-source graphics and game engine written in C++.

The project is being built step by step with a small core, platform-specific code behind abstractions, renderer-specific code behind a common interface, and no mandatory dependency on a store, account system, launcher or online service.

> **Current version:** `0.0.7`
>
> **Codename:** `BIG BEAR GUMMY`
>
> **Status:** early prototype — not production ready.

## What 0.0.7 changes

Jevaing 0.0.7 adds the first real 3D foundation while preserving the existing 2D path.

The public API now includes `Vec3`, `Mat4`, `Transform`, `PerspectiveCamera` and `Graphics3D`. The DirectX 11 backend creates a depth buffer, uploads a model-view-projection matrix through a constant buffer, and can draw a minimal colored cube.

The Sandbox is now an interactive 3D client: `WASD` moves the cube, arrow keys rotate it, `Space` changes its color, and `ESC` closes the window.

The new 3D smoke test is available with:

```powershell
.\bin\Debug\JevaingSandbox.exe --graphics-test-3d
```

## What 0.0.6 changed

BIG BEAR GUMMY is the first Jevaing version aimed at being a base for writing an actual client program instead of only proving that individual engine subsystems work.

The `Sandbox` is now a real client of the public Jevaing API. It subclasses `Jevaing::Game`, receives update/render callbacks, reads keyboard input and submits simple 2D drawing commands without touching Win32 or Direct3D directly.

The current Windows build provides:

- C++17 core.
- CMake build system.
- Public `Game` lifecycle API.
- Public `GameConfig` window configuration.
- `OnStart`, `OnUpdate`, `OnRender`, `OnResize` and `OnStop` callbacks.
- Generic engine-facing window abstraction.
- Native Win32 window implementation.
- Keyboard input state for WASD, arrows, Space, Enter and Escape.
- `IsKeyDown`, `IsKeyPressed` and `IsKeyReleased` queries.
- Engine timer and delta time.
- Generic renderer abstraction.
- Null Renderer for architecture/testing work.
- DirectX 11 renderer on Windows.
- Runtime HLSL compilation.
- Dynamic 2D vertex buffer.
- Immediate colored triangle and quad drawing through the public API.
- Swap-chain/render-target recreation when the client area changes size.
- Existing ATLAS triangle and penguin graphics tests.
- Headless core self-tests.
- Fixed-frame smoke tests suitable for scripts and future CI.
- An interactive Sandbox demo built only on the public Jevaing API.

Vulkan, Metal, Linux and macOS are still planned rather than implemented.

## Why BIG BEAR GUMMY matters

Before 0.0.6 the main execution path was primarily an engine bring-up path:

```text
Application
    |
    +---- Window
    |
    +---- Renderer
             |
             +---- built-in graphics test
```

BIG BEAR GUMMY introduces a client layer:

```text
Sandbox Game
     |
     v
Jevaing public API
     |
     v
Application runtime
  /      |       \
 v       v        v
Input   Game    Graphics2D
 |       |        |
 v       v        v
Win32  callbacks Renderer
                 |
                 v
             DirectX 11
```

A client can now own its state, update it with `deltaTime`, read input and submit simple graphics commands without including Windows or DirectX headers.

That is intentionally still a small API. The goal is to establish a usable foundation before adding scenes, entities, textures, audio or an editor.

## Interactive Sandbox

Build and run Jevaing normally:

```powershell
git clone https://github.com/jesunixtux/jevaing.git
cd jevaing
cmake -S . -B build
cmake --build build --config Debug
.\bin\Debug\JevaingSandbox.exe
```

The default Sandbox should open a window titled:

```text
Jevaing 0.0.7 - BIG BEAR GUMMY Sandbox
```

It displays a colored 3D cube drawn through the public `Graphics3D` API, with a small 2D overlay drawn through `Graphics2D`.

Controls:

| Input | Action |
|---|---|
| `WASD` | Move the cube |
| Arrow keys | Rotate the cube |
| `Space` | Change the cube color while held |
| `ESC` | Close the window |

The demo is deliberately simple. Its purpose is to prove this path:

```text
Sandbox state
    -> OnUpdate(deltaTime)
    -> Jevaing::Input
    -> OnRender(Graphics2D&) / OnRender(Graphics3D&)
    -> DrawQuad / DrawTriangle / DrawCube
    -> DirectX 11
    -> GPU
```

## Public client API

The current client-facing structure is intentionally small.

A minimal client can look like this:

```cpp
#include <Jevaing/Jevaing.h>

class MyGame final : public Jevaing::Game
{
public:
    void OnUpdate(double deltaTime) override
    {
        if (Jevaing::Input::IsKeyDown(Jevaing::Key::D))
        {
            x += static_cast<float>(deltaTime);
        }
    }

    void OnRender(Jevaing::Graphics2D& graphics) override
    {
        graphics.Clear({ 0.02f, 0.03f, 0.05f, 1.0f });

        graphics.DrawQuad(
            { x, 0.0f },
            { 0.20f, 0.20f },
            { 1.0f, 0.45f, 0.15f, 1.0f }
        );
    }

private:
    float x = 0.0f;
};

int main(int argc, char** argv)
{
    MyGame game;

    Jevaing::GameConfig config;
    config.Title = "My Jevaing Game";
    config.Width = 1280;
    config.Height = 720;

    return Jevaing::Run(game, config, argc, argv);
}
```

The public API currently includes:

```text
Jevaing::Game
Jevaing::GameConfig
Jevaing::Graphics2D
Jevaing::Graphics3D
Jevaing::Vec2
Jevaing::Vec3
Jevaing::Mat4
Jevaing::Transform
Jevaing::PerspectiveCamera
Jevaing::Color
Jevaing::Key
Jevaing::Input
Jevaing::Run(...)
Jevaing::GetVersion()
Jevaing::GetCodename()
```

These APIs may change while Jevaing is below version 1.0.

## Game lifecycle

A client `Game` can override:

```cpp
void OnStart();
void OnUpdate(double deltaTime);
void OnRender(Jevaing::Graphics2D& graphics);
void OnRender(Jevaing::Graphics3D& graphics);
void OnResize(int width, int height);
void OnStop();
```

The normal runtime order is approximately:

```text
create Window
create Renderer
OnStart
OnResize(initial size)

loop:
    snapshot Input
    process platform events
    resize renderer if needed
    OnResize if needed
    calculate deltaTime
    OnUpdate(deltaTime)
    BeginFrame
    OnRender(Graphics2D)
    OnRender(Graphics3D)
    EndFrame / Present

OnStop
```

## Input foundation

BIG BEAR GUMMY exposes basic keyboard state through:

```cpp
Jevaing::Input::IsKeyDown(key);
Jevaing::Input::IsKeyPressed(key);
Jevaing::Input::IsKeyReleased(key);
```

Currently mapped keys:

```text
W A S D
Up Down Left Right
Space
Enter
Escape
```

The Win32 backend owns the platform key-message translation. Client code only sees `Jevaing::Key`.

Mouse/gamepad input is not implemented yet.

## Graphics2D foundation

The public renderer-facing 2D API currently supports:

```cpp
graphics.Clear(color);
graphics.DrawTriangle(a, b, c, color);
graphics.DrawQuad(center, size, color);
```

Coordinates currently use normalized device-style coordinates:

```text
(-1, +1)                (+1, +1)
     +--------------------+
     |                    |
     |       (0,0)        |
     |                    |
     +--------------------+
(-1, -1)                (+1, -1)
```

This is a temporary low-level coordinate convention. A future camera/2D world system should sit above it rather than forcing game code to live permanently in normalized coordinates.

`Graphics2D` is backend-neutral. The current implementation is DirectX 11, but client code does not include D3D11 types.

### Dynamic geometry

BIG BEAR GUMMY replaces the fixed ATLAS triangle vertex buffer with a dynamic 2D vertex buffer.

Each frame can submit multiple triangles/quads. The DirectX backend grows its dynamic buffer when the current capacity is insufficient, uploads the frame vertices and performs the draw call before presenting the swap chain.

This is still an immediate-mode prototype, not a final batching or material system.

## Window resizing

The generic `Window` interface can now report the current client width/height.

When the Win32 client area changes size, the runtime detects the new dimensions and asks the renderer to resize. The DirectX 11 backend releases its old render-target view, resizes the swap-chain buffers, recreates the render target and updates the viewport.

Client games receive `OnResize(width, height)` after a successful renderer resize.

This is an important foundation for future cameras, UI and resolution handling.

## Command-line testing

The command-line testing introduced in ARPA+ remains available.

### Help

```powershell
.\bin\Debug\JevaingSandbox.exe --help
```

### Version

```powershell
.\bin\Debug\JevaingSandbox.exe --version
```

Expected:

```text
Jevaing 0.0.7 - BIG BEAR GUMMY
```

### Headless core tests

```powershell
.\bin\Debug\JevaingSandbox.exe --self-test
$LASTEXITCODE
```

A healthy run returns `0`.

The self-test currently validates the version/codename, timer, 3D math/camera basics, renderer parsing/availability, initial input state and default `GameConfig` dimensions.

### Renderer information

```powershell
.\bin\Debug\JevaingSandbox.exe --renderer-info
```

Expected on Windows:

```text
None: available
DirectX: available
Vulkan: not available
Metal: not available
Default renderer: DirectX
```

### Triangle graphics test

```powershell
.\bin\Debug\JevaingSandbox.exe --graphics-test
```

### Penguin graphics test

```powershell
.\bin\Debug\JevaingSandbox.exe --graphics-test-penguin
```

### 3D cube graphics test

```powershell
.\bin\Debug\JevaingSandbox.exe --graphics-test-3d
```

### Client runtime smoke test

```powershell
.\bin\Debug\JevaingSandbox.exe --runtime-test
```

This opens the real Sandbox client, exercises its `OnUpdate` and `OnRender` callbacks for 120 frames and exits automatically.

A healthy run ends with:

```text
[PASS] BIG BEAR GUMMY client runtime callbacks completed.
```

### Fixed frame count

Any normal client run can also be bounded:

```powershell
.\bin\Debug\JevaingSandbox.exe --frames 300
```

or:

```powershell
.\bin\Debug\JevaingSandbox.exe --runtime-test --frames 600
```

### Force a renderer

```powershell
.\bin\Debug\JevaingSandbox.exe --renderer directx
.\bin\Debug\JevaingSandbox.exe --renderer null --frames 60
```

Unavailable backends fail explicitly rather than silently falling back:

```powershell
.\bin\Debug\JevaingSandbox.exe --renderer vulkan
```

## Dependencies

### Required on Windows

- CMake 3.20 or newer.
- C++17-compatible compiler.
- Windows SDK.
- MSVC / Visual Studio Build Tools or another compatible Windows C++ toolchain.
- Git is recommended.

### Windows libraries

The current Windows implementation links system libraries only:

```text
user32
gdi32
d3d11
dxgi
d3dcompiler
```

There is currently no SDL, GLFW, Qt, DirectXTK or third-party engine/runtime dependency.

## Repository structure

```text
jevaing/
├── Api/
│   └── Include/Jevaing/
│       ├── Game.h
│       ├── Graphics2D.h
│       ├── Input.h
│       ├── Jevaing.h
│       └── Types.h
│
├── Engine/
│   ├── Core/
│   │   ├── Application.*
│   │   ├── CommandLine.*
│   │   ├── Input.*
│   │   ├── InputState.h
│   │   ├── Logger.*
│   │   ├── Timer.*
│   │   ├── Version.*
│   │   └── Window.*
│   │
│   ├── Platform/
│   │   ├── Windows/
│   │   ├── Linux/       # planned
│   │   └── MacOS/       # planned
│   │
│   └── Renderer/
│       ├── Renderer.*
│       ├── DirectX/
│       │   └── D3D11Renderer.*
│       ├── Vulkan/      # planned
│       └── Metal/       # planned
│
├── Sandbox/
│   └── main.cpp
├── CMakeLists.txt
├── LICENSE
└── README.md
```

## Renderer backends

| Backend | Status | Current capability |
|---|---|---|
| Null | Working | Client/runtime testing without GPU work |
| DirectX 11 | Working prototype | Clear, dynamic colored 2D triangles/quads, depth-buffered 3D cube, resize, present |
| Vulkan | Planned | Not implemented |
| Metal | Planned | Not implemented |

## Platform plan

| Platform | Architecture | Status | Intended graphics backend |
|---|---|---|---|
| Windows | x64 | Prototype working | DirectX / Vulkan |
| Windows | ARM64 | Planned | DirectX / Vulkan |
| Linux | x64 | Planned | Vulkan |
| Linux | ARM64 | Planned | Vulkan |
| macOS | Apple Silicon / ARM64 | Planned | Metal |

## Roadmap

### 0.0.1 — RENACO

- [x] CMake/C++ core.
- [x] Native Win32 window.
- [x] Basic event loop and shutdown.

### 0.0.2 — MARIA

- [x] Generic window abstraction.
- [x] Logger.
- [x] Timer and delta time.

### 0.0.3 — ARPA

- [x] Common renderer interface.
- [x] Null Renderer.
- [x] Renderer lifecycle.

### 0.0.4 — ARPA+

- [x] DirectX 11 device and swap chain.
- [x] GPU clear/present.
- [x] CLI/self-test foundation.

### 0.0.5 — ATLAS

- [x] Runtime HLSL compilation.
- [x] Vertex/pixel shaders.
- [x] First GPU triangle.
- [x] Penguin geometry test.

### 0.0.6 — BIG BEAR GUMMY

- [x] Public client `Game` lifecycle.
- [x] Public `GameConfig`.
- [x] Basic keyboard input API.
- [x] Per-frame key down/pressed/released state.
- [x] Public backend-neutral `Graphics2D` API.
- [x] Dynamic 2D vertex buffer.
- [x] `DrawTriangle`.
- [x] `DrawQuad`.
- [x] Renderer/window resize path.
- [x] `OnResize` callback.
- [x] Interactive Sandbox client.
- [x] Runtime callback smoke test.

### 0.0.7 - BIG BEAR GUMMY

- [x] Public `Vec3`, `Mat4`, `Transform` and `PerspectiveCamera`.
- [x] Public backend-neutral `Graphics3D` API.
- [x] DirectX 11 depth buffer.
- [x] DirectX 11 model-view-projection constant buffer.
- [x] Minimal `DrawCube`.
- [x] `--graphics-test-3d` cube smoke test.
- [x] Interactive 3D Sandbox client.

### Next useful foundations

- [ ] Mouse input.
- [ ] Gamepad input.
- [ ] Textures and image loading.
- [ ] Sprite drawing.
- [ ] 2D camera/world coordinates.
- [ ] Asset handles/cache.
- [ ] Scene foundation.
- [ ] Entity/component design.
- [ ] Audio layer.
- [ ] Vulkan backend.
- [ ] Metal backend.
- [ ] Automated CI on supported platforms.

## Current limitations

BIG BEAR GUMMY is a development foundation, not a complete game engine.

Not implemented yet:

- textures/sprites;
- font/text rendering;
- mouse/gamepad input;
- audio;
- scene/entity system;
- asset management;
- camera/world-space renderer;
- physics;
- scripting;
- editor;
- Vulkan/Metal;
- Linux/macOS platform windows.

The immediate 2D API is intentionally small so these systems can be added on top of a working runtime instead of designing them in isolation.

## Design principles

- Core/client code should not require OS-specific types.
- Platform code belongs in `Engine/Platform`.
- Renderer backend code belongs in `Engine/Renderer`.
- The public API should expose useful engine concepts rather than Win32/D3D details.
- Unsupported backends should fail explicitly.
- New systems should come with something concrete that can be run or tested.
- Store/account/platform integrations should remain optional.

## Versioning

Jevaing is pre-alpha and uses `0.x.x` versions.

Until `1.0`, public APIs, file layouts and internal architecture may change without backwards-compatibility guarantees.

Development releases currently use codenames including `RENACO`, `MARIA`, `ARPA`, `ARPA+`, `ATLAS` and `BIG BEAR GUMMY`.

## License

Jevaing is licensed under the **MIT License**.

Copyright (c) 2026 jesunixtux.

See [`LICENSE`](LICENSE) for the full license text.
