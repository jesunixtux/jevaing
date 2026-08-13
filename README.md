# Jevaing

Jevaing is an experimental open-source graphics and game engine written in C++.

The project is being built step by step, with a small and understandable core, platform-specific code kept behind abstractions, and no mandatory dependency on a store, account system, online service, or vendor platform.

> **Current version:** `0.0.3`
>
> **Codename:** `ARPA`
>
> **Status:** early prototype — not production ready.

## What works today

Jevaing 0.0.3 ARPA currently provides a small but functional engine foundation on Windows:

- C++17 core.
- CMake build system.
- Minimal public Jevaing API.
- Generic engine-facing window abstraction.
- Platform window factory.
- Native Win32 window implementation hidden behind the generic window API.
- UTF-8 engine window titles converted internally to Win32 UTF-16.
- Native Windows message loop.
- Window move, resize, minimize and maximize support.
- Exit through the window close button or `ESC`.
- Dark native window background.
- Logger with info, warning and error levels.
- Engine timer based on `std::chrono::steady_clock`.
- Per-loop delta time measurement.
- Generic renderer interface.
- Renderer backend selection enum/configuration.
- Renderer-independent `BeginFrame()` / `EndFrame()` boundaries.
- A functional **Null Renderer** used to validate the renderer architecture without pretending a GPU backend already exists.
- Debug and Release output directories.
- Separation between Sandbox, Core, Platform and Renderer code.

ARPA still has **no real GPU renderer**. DirectX, Vulkan and Metal are planned backends but are not implemented yet.

The current Null Renderer intentionally draws nothing. The black window background still comes from the Win32 platform layer.

Linux and macOS support are planned but are not functional yet.

## Project goals

Jevaing is designed around a few simple rules:

- Keep the Core independent from operating-system-specific code whenever possible.
- Keep rendering backends separate from the rest of the engine.
- Allow different graphics APIs depending on the target platform.
- Avoid mandatory accounts, stores, launchers or online services.
- Allow external platform integrations through optional layers or plugins in the future.
- Keep the engine usable for both open-source and commercial projects under the MIT License.
- Add systems only when they are needed and can be tested.

The long-term goal is that game and engine systems should not need to know whether Jevaing is running through Win32, Linux or macOS, or whether rendering is provided by DirectX, Vulkan or Metal.

## Current architecture

ARPA adds the first renderer abstraction on top of the platform foundation introduced in MARIA:

```text
Sandbox/main.cpp
        |
        v
   Jevaing::Run()
        |
        v
    Application
      /      \
     v        v
  Window    Renderer
     |         |
     v         v
Platform    Renderer factory
 factory       |
     |         v
     v      Null Renderer
WindowsWindow  |
     |         +---- future: DirectX
     v         +---- future: Vulkan
   Win32       +---- future: Metal
```

`Application.cpp` does not construct `WindowsWindow` directly and does not contain DirectX, Vulkan or Metal code.

The Core asks `Window::Create(...)` for a platform window and `Renderer::Create(...)` for a renderer. ARPA currently selects the Null Renderer so the complete renderer lifecycle can be exercised before a real graphics backend is introduced.

The public Sandbox still only needs the public API:

```cpp
#include <Jevaing/Jevaing.h>

int main()
{
    return Jevaing::Run();
}
```

The current public API is intentionally tiny:

```cpp
Jevaing::GetVersion();
Jevaing::GetCodename();
Jevaing::Run();
```

This API may change while Jevaing is below version 1.0.

## Repository structure

```text
jevaing/
├── Api/
│   └── Include/
│       └── Jevaing/
│           └── Jevaing.h
│
├── Engine/
│   ├── Core/
│   │   ├── Application.*
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
│       ├── DirectX/     # planned
│       ├── Vulkan/      # planned
│       └── Metal/       # planned
│
├── Library/             # future external dependencies
├── Sandbox/             # small program used to validate the engine
├── bin/
│   ├── Debug/
│   └── Release/
│
├── CMakeLists.txt
├── LICENSE
└── README.md
```

Some planned directories may not appear in Git until they contain tracked files.

## Dependencies

### Required to build ARPA on Windows

- **CMake 3.20 or newer**.
- A **C++17-compatible compiler**.
- **Windows SDK** with Win32 development headers and libraries.
- A supported Windows C++ toolchain such as **MSVC / Visual Studio Build Tools**.
- Git is recommended for cloning and updating the repository.

### Windows system libraries currently used

ARPA currently links only against libraries provided by Windows:

- `user32`
- `gdi32`

No third-party runtime library is currently required.

Jevaing does **not** currently depend on SDL, GLFW, Qt, DirectXTK or another game engine/framework.

No DirectX, Vulkan or Metal SDK integration is required yet because ARPA uses the Null Renderer.

## Building on Windows

Clone the repository:

```powershell
git clone https://github.com/jesunixtux/jevaing.git
cd jevaing
```

Generate the build files:

```powershell
cmake -S . -B build
```

Build the Debug configuration:

```powershell
cmake --build build --config Debug
```

Run ARPA:

```powershell
.\bin\Debug\JevaingSandbox.exe
```

A window titled:

```text
Jevaing 0.0.3 - ARPA
```

should appear.

The console should include a line similar to:

```text
[Jevaing][INFO] Renderer initialized: Null Renderer [None]
```

That line confirms that `Application -> Renderer -> Null Renderer` is working.

Close the window with the title-bar close button or press `ESC`.

### Release build

```powershell
cmake --build build --config Release
```

The executable will be placed in:

```text
bin/Release/
```

## Cleaning generated files

Generated build files are intentionally kept out of source control.

To remove the local CMake build directory from PowerShell:

```powershell
Remove-Item .\build -Recurse -Force
```

Regenerate it with:

```powershell
cmake -S . -B build
```

Visual Studio local metadata, CMake build output and compiled binaries are ignored through `.gitignore`.

## Platform plan

| Platform | Architecture | Status | Intended graphics backend |
|---|---|---|---|
| Windows | x64 | Basic platform layer working | DirectX / Vulkan |
| Windows | ARM64 | Planned | DirectX / Vulkan |
| Linux | x64 | Planned | Vulkan |
| Linux | ARM64 | Planned | Vulkan |
| macOS | Apple Silicon / ARM64 | Planned | Metal |

Support listed as **planned** is not available yet.

## Rendering foundation

ARPA introduces the common renderer-facing API.

The current renderer lifecycle is intentionally small:

```text
Renderer::Create(...)
        |
        v
    Initialize
        |
        v
  BeginFrame()
        |
        v
future update/render work
        |
        v
   EndFrame()
```

The currently available backend is:

```text
RendererBackend::None
        |
        v
   Null Renderer
```

The Null Renderer does not use the GPU. Its purpose is to prove that the engine loop can operate through a renderer-independent interface.

The planned evolution is:

```text
Game / Engine systems
        |
        v
    Renderer API
    /    |     \
   v     v      v
DirectX Vulkan Metal
```

Backend requests for DirectX, Vulkan or Metal currently fail explicitly instead of silently pretending that those renderers exist.

## Platform integration philosophy

Jevaing itself should not require a specific commercial platform.

Future integrations may include achievements, cloud saves, multiplayer services, storefront APIs, authentication, and mod/workshop services.

These should be optional integrations rather than requirements of the Core. A project should eventually be able to use Steam, Epic, GOG, a console service, a custom backend, or no online platform at all without replacing the engine core.

## Roadmap

This roadmap is intentionally flexible. Items may move as the engine architecture becomes clearer.

### 0.0.1 — RENACO

- [x] Initial repository structure.
- [x] MIT License.
- [x] CMake project.
- [x] C++17 core.
- [x] Minimal public API.
- [x] Sandbox executable.
- [x] Native Win32 window.
- [x] Native Windows event loop.
- [x] Window resizing and standard window controls.
- [x] Exit through the close button.
- [x] Exit with `ESC`.
- [x] Dark native background.
- [x] Basic startup/shutdown logs.

### 0.0.2 — MARIA

- [x] Generic cross-platform window interface.
- [x] Platform window factory.
- [x] Remove direct Win32 dependency from `Application.cpp`.
- [x] Generic window configuration with title, width and height.
- [x] UTF-8 title handling at the Core boundary.
- [x] Basic logging system with info, warning and error levels.
- [x] Engine timer.
- [x] Delta time measurement.
- [x] Keep the existing Win32 behavior from RENACO.

### 0.0.3 — ARPA

- [x] Common renderer interface.
- [x] Renderer backend enum and configuration.
- [x] Renderer factory.
- [x] Null Renderer for renderer-independent testing.
- [x] Renderer initialization integrated into `Application`.
- [x] `BeginFrame()` / `EndFrame()` lifecycle integrated into the engine loop.
- [x] Explicit failure for unimplemented GPU backends.
- [x] Preserve the window, logger and timer behavior from MARIA.

### Core foundation

- [x] Generic cross-platform window abstraction.
- [x] Engine timer and delta time.
- [x] Logging system with info, warning and error levels.
- [x] Common renderer interface.
- [ ] Basic keyboard and mouse input API.
- [ ] Basic game/application loop separation.
- [ ] Automated tests for core systems.

### Platform support

- [ ] Linux platform layer.
- [ ] Linux x64 builds.
- [ ] Linux ARM64 builds.
- [ ] macOS platform layer.
- [ ] macOS ARM64 builds.
- [ ] Windows ARM64 build validation.

### Graphics

- [x] Common renderer interface.
- [ ] DirectX backend.
- [ ] Vulkan backend.
- [ ] Metal backend.
- [ ] First GPU triangle.
- [ ] GPU buffer management.
- [ ] Shader loading and compilation workflow.
- [ ] Texture loading.
- [ ] Basic 2D rendering.
- [ ] Camera system.
- [ ] Basic 3D rendering.

### Engine systems

- [ ] Asset system.
- [ ] Scene system.
- [ ] Entity/component architecture.
- [ ] Audio layer.
- [ ] Physics integration or native physics layer.
- [ ] Scripting layer.
- [ ] Save/load foundation.
- [ ] Plugin system.

### Tools and developer experience

- [ ] Example projects.
- [ ] Better API documentation.
- [ ] Build presets.
- [ ] Continuous integration for supported platforms.
- [ ] Debug tooling.
- [ ] Editor prototype.

## Versioning

Jevaing is currently pre-alpha and uses `0.x.x` versions.

Until version `1.0`, the public API, file structure and internal architecture may change without backwards compatibility guarantees.

Development versions currently include:

- `0.0.1` — RENACO
- `0.0.2` — MARIA
- `0.0.3` — ARPA

## Contributing

Jevaing is intended to remain understandable and modular.

When contributing, prefer small changes that can be tested independently. Platform-specific code should stay inside the appropriate platform layer, and renderer-specific code should stay inside its renderer backend whenever possible.

Avoid introducing mandatory vendor services or large dependencies into the Core without a clear reason.

## License

Jevaing is licensed under the **MIT License**.

Copyright (c) 2026 jesunixtux.

See [`LICENSE`](LICENSE) for the full license text.
