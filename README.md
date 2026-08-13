# Jevaing

Jevaing is an experimental open-source graphics and game engine written in C++.

The project is currently in a very early stage. The goal is to build the engine step by step, keeping the core small, understandable, modular, and independent from any store, account system, online service, or vendor-specific platform.

> **Current version:** `0.0.1`
>
> **Codename:** `RENACO`
>
> **Status:** early prototype — not production ready.

## What works today

Jevaing 0.0.1 RENACO currently provides a minimal working engine bootstrap on Windows:

- C++17 core.
- CMake build system.
- Minimal public Jevaing API.
- Native Win32 window creation.
- 1280x720 test window.
- Native Windows message loop.
- Window move, resize, minimize and maximize support through Win32.
- Exit through the window close button.
- Exit with the `ESC` key.
- Dark native window background.
- Basic startup and shutdown logging.
- Debug and Release output directories.
- Clean separation between the Sandbox application and the internal engine code.

There is **no graphics renderer yet**. DirectX, Vulkan and Metal are planned backends and are not implemented in RENACO.

Linux and macOS support are also planned but are not functional yet.

## Project goals

Jevaing is being designed around a few simple rules:

- Keep the core independent from operating-system-specific code whenever possible.
- Keep rendering backends separate from the rest of the engine.
- Allow different graphics APIs depending on the target platform.
- Avoid mandatory accounts, stores, launchers or online services.
- Allow external platform integrations through optional layers or plugins in the future.
- Keep the engine usable for both open-source and commercial projects under the MIT License.
- Add systems only when they are actually needed and testable.

The long-term idea is that a game should not need to know whether Jevaing is running on Win32, Linux or macOS, or whether rendering is handled by DirectX, Vulkan or Metal.

## Current architecture

The current prototype follows this basic flow:

```text
Sandbox/main.cpp
        |
        v
   Jevaing::Run()
        |
        v
    Application
        |
        v
   WindowsWindow
        |
        v
      Win32
```

The public Sandbox currently only needs the public API:

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

This API will change while Jevaing is below version 1.0.

## Repository structure

The project is organized around a small number of responsibilities:

```text
jevaing/
├── Api/
│   └── Include/
│       └── Jevaing/
│           └── Jevaing.h
│
├── Engine/
│   ├── Core/
│   ├── Platform/
│   │   ├── Windows/
│   │   ├── Linux/       # planned
│   │   └── MacOS/       # planned
│   │
│   └── Renderer/
│       ├── DirectX/     # planned
│       ├── Vulkan/      # planned
│       └── Metal/       # planned
│
├── Library/             # future external dependencies
├── Sandbox/             # small program used to test the engine
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

### Required to build RENACO on Windows

- **CMake 3.20 or newer**.
- A **C++17-compatible compiler**.
- **Windows SDK** with Win32 development headers and libraries.
- A supported Windows C++ toolchain such as **MSVC / Visual Studio Build Tools**.
- Git is recommended for cloning and updating the repository.

### Windows system libraries currently used

RENACO links only against libraries provided by Windows:

- `user32`
- `gdi32`

No third-party runtime library is currently required.

Jevaing does **not** currently depend on SDL, GLFW, Qt, DirectXTK or another game engine/framework.

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

Run RENACO:

```powershell
.\bin\Debug\JevaingSandbox.exe
```

A window titled:

```text
Jevaing 0.0.1 - RENACO
```

should appear.

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

You can then regenerate it with:

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

## Rendering plan

Jevaing aims to keep rendering behind a common engine-facing API.

The intended design is roughly:

```text
Game / Engine systems
        |
        v
   Renderer API
    /    |    \
   v     v     v
DirectX Vulkan Metal
```

The renderer abstraction does not exist yet in RENACO. It will be introduced gradually instead of creating a large unused architecture upfront.

## Platform integration philosophy

Jevaing itself should not require a specific commercial platform.

Future integrations may include things such as:

- achievements;
- cloud saves;
- multiplayer services;
- storefront APIs;
- authentication;
- mod or workshop services.

These should be optional integrations rather than requirements of the core engine.

A project should eventually be able to use Steam, Epic, GOG, a console service, a custom backend, or no online platform at all without replacing the engine core.

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

### Core foundation

- [ ] Generic cross-platform window abstraction.
- [ ] Engine timer and delta time.
- [ ] Logging system with info, warning and error levels.
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

- [ ] Common renderer interface.
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

Development versions may also use internal codenames such as `RENACO`.

## Contributing

Jevaing is intended to remain understandable and modular.

When contributing, prefer small changes that can be tested independently. Platform-specific code should stay inside the appropriate platform layer, and renderer-specific code should stay inside its renderer backend whenever possible.

Avoid introducing mandatory vendor services or large dependencies into the core without a clear reason.

## License

Jevaing is licensed under the **MIT License**.

Copyright (c) 2026 jesunixtux.

See [`LICENSE`](LICENSE) for the full license text.
