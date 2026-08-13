# Jevaing

Jevaing is an experimental open-source graphics and game engine written in C++.

The project is built step by step with a small, understandable core, platform-specific code behind abstractions, and no mandatory dependency on a store, account system, online service, or vendor platform.

> **Current version:** `0.0.5`
>
> **Codename:** `ATLAS`
>
> **Status:** early prototype — not production ready.

## What works today

Jevaing 0.0.5 ATLAS currently provides a small but functional engine foundation on Windows:

- C++17 core.
- CMake build system.
- Minimal public Jevaing API.
- Generic engine-facing window abstraction.
- Platform window factory.
- Native Win32 window implementation hidden behind the generic window API.
- UTF-8 window titles converted internally to Win32 UTF-16.
- Native Windows event loop.
- Window move, resize, minimize and maximize support.
- Exit through the close button or `ESC`.
- Logger with info, warning and error levels.
- Engine timer based on `std::chrono::steady_clock`.
- Delta-time measurement.
- Generic renderer interface.
- Renderer-independent `BeginFrame()` / `EndFrame()` boundaries.
- Null Renderer for architecture and fallback testing.
- **DirectX 11 renderer on Windows.**
- DirectX 11 device, swap chain and render-target creation.
- Runtime HLSL shader compilation.
- Vertex shader and pixel shader creation.
- Input layout and immutable vertex buffer.
- Viewport setup.
- **First GPU triangle rendered by Jevaing.**
- Command-line test interface.
- Headless core self-tests that do not open a window.
- Automatic frame-limited runs for repeatable renderer tests.
- `--graphics-test` GPU pipeline smoke test.

Vulkan and Metal are represented in the renderer API but are **not implemented yet**.

Linux and macOS platform implementations are also planned but are not functional yet.

## 0.0.5 ATLAS goal

ATLAS is the first Jevaing version that sends its own geometry through a real graphics pipeline.

The Windows graphics path is now:

```text
Sandbox
   |
   v
Jevaing::Run(argc, argv)
   |
   v
Application
  /     \
 v       v
Window  Renderer
 |        |
 v        v
Win32   DirectX 11
          |
          v
      HLSL shaders
          |
          v
     Vertex buffer
          |
          v
     Draw(3, 0)
          |
          v
     Colored triangle
```

The triangle is intentionally simple. It is renderer bring-up geometry, not yet a public mesh or draw-command API. Its purpose is to prove that the complete DirectX 11 path works from Jevaing's engine loop to the GPU.

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
│   │   ├── CommandLine.*
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
├── Library/
├── Sandbox/
├── bin/
├── CMakeLists.txt
├── LICENSE
└── README.md
```

## Dependencies

### Required to build ATLAS on Windows

- **CMake 3.20 or newer**.
- A **C++17-compatible compiler**.
- **Windows SDK** with Win32 and Direct3D 11 development files.
- A Windows C++ toolchain such as **MSVC / Visual Studio Build Tools**.
- Git is recommended.

### Windows libraries currently linked

Jevaing currently uses Windows-provided libraries only:

- `user32`
- `gdi32`
- `d3d11`
- `dxgi`
- `d3dcompiler`

`d3dcompiler` is used by ATLAS to compile the small embedded HLSL bring-up shader at runtime.

No SDL, GLFW, Qt, DirectXTK, third-party game engine or third-party runtime is required.

## Building on Windows

Clone the repository:

```powershell
git clone https://github.com/jesunixtux/jevaing.git
cd jevaing
```

Generate build files:

```powershell
cmake -S . -B build
```

Build Debug:

```powershell
cmake --build build --config Debug
```

Run normally:

```powershell
.\bin\Debug\JevaingSandbox.exe
```

The window title should be:

```text
Jevaing 0.0.5 - ATLAS
```

On Windows with the DirectX backend, the window should show a colored triangle over a dark blue background.

The console should include lines similar to:

```text
[Jevaing][INFO] DirectX 11 device, swap chain and ATLAS triangle pipeline initialized.
[Jevaing][INFO] Renderer initialized: DirectX 11 Renderer [DirectX]
[Jevaing][INFO] Engine initialized.
```

## Command-line testing

ATLAS keeps the ARPA+ command-line test interface and adds a graphics-pipeline smoke test.

### Show available commands

```powershell
.\bin\Debug\JevaingSandbox.exe --help
```

### Print the current version

```powershell
.\bin\Debug\JevaingSandbox.exe --version
```

Expected:

```text
Jevaing 0.0.5 - ATLAS
```

### Run core self-tests

```powershell
.\bin\Debug\JevaingSandbox.exe --self-test
```

This does **not** open a window. A healthy run returns exit code `0`.

### Inspect renderer availability

```powershell
.\bin\Debug\JevaingSandbox.exe --renderer-info
```

On the current Windows build, the expected state is approximately:

```text
None: available
DirectX: available
Vulkan: not available
Metal: not available
Default renderer: DirectX
```

### Run the ATLAS graphics smoke test

```powershell
.\bin\Debug\JevaingSandbox.exe --graphics-test
```

On Windows this selects the default DirectX renderer, creates the complete ATLAS shader/input-layout/vertex-buffer pipeline, renders the colored triangle for 180 frames and exits automatically.

A successful run ends with:

```text
[Jevaing][INFO] [PASS] ATLAS graphics pipeline smoke test completed.
```

You can inspect the exit code with:

```powershell
$LASTEXITCODE
```

Expected after a successful graphics test:

```text
0
```

You can override the frame count:

```powershell
.\bin\Debug\JevaingSandbox.exe --graphics-test --frames 600
```

### Force DirectX 11

```powershell
.\bin\Debug\JevaingSandbox.exe --renderer directx
```

Aliases accepted for this backend include `dx11` and `d3d11`.

### Force the Null Renderer

```powershell
.\bin\Debug\JevaingSandbox.exe --renderer null
```

The Null Renderer performs no GPU rendering and remains useful for testing the platform/event-loop layer independently.

`--graphics-test` intentionally rejects the Null Renderer because the graphics smoke test must exercise a real GPU backend.

### Run a fixed number of frames

```powershell
.\bin\Debug\JevaingSandbox.exe --renderer directx --frames 300
```

Jevaing opens the window, renders 300 frames, shuts down and returns control to PowerShell automatically.

### Test an unavailable backend

```powershell
.\bin\Debug\JevaingSandbox.exe --renderer vulkan
```

ATLAS should fail explicitly and return a non-zero exit code instead of silently falling back.

## Release build

```powershell
cmake --build build --config Release
```

The executable is placed in:

```text
bin/Release/
```

## Renderer backends

| Backend | Status | Notes |
|---|---|---|
| Null | Working | No GPU work; architecture/testing backend |
| DirectX 11 | Working prototype | Device + swap chain + shaders + vertex buffer + first triangle |
| Vulkan | Planned | Not implemented |
| Metal | Planned | Not implemented |

The renderer-facing API is intended to stay independent of the backend:

```text
Application / engine systems
            |
            v
         Renderer
      /      |      \
     v       v       v
 DirectX   Vulkan   Metal
```

## Platform plan

| Platform | Architecture | Status | Intended graphics backend |
|---|---|---|---|
| Windows | x64 | Prototype working | DirectX / Vulkan |
| Windows | ARM64 | Planned | DirectX / Vulkan |
| Linux | x64 | Planned | Vulkan |
| Linux | ARM64 | Planned | Vulkan |
| macOS | Apple Silicon / ARM64 | Planned | Metal |

## Roadmap

The roadmap is intentionally flexible.

### 0.0.1 — RENACO

- [x] Initial repository structure.
- [x] MIT License.
- [x] CMake project.
- [x] Minimal public API.
- [x] Native Win32 window.
- [x] Windows event loop.
- [x] Basic clean shutdown.

### 0.0.2 — MARIA

- [x] Generic window abstraction.
- [x] Platform window factory.
- [x] Remove direct Win32 dependency from `Application.cpp`.
- [x] Logger.
- [x] Timer and delta time.

### 0.0.3 — ARPA

- [x] Common renderer interface.
- [x] Renderer backend enum/configuration.
- [x] Renderer factory.
- [x] `BeginFrame()` / `EndFrame()` lifecycle.
- [x] Null Renderer.
- [x] Explicit errors for unimplemented backends.

### 0.0.4 — ARPA+

- [x] DirectX 11 backend on Windows.
- [x] Opaque native-window handle for renderer integration.
- [x] D3D11 device creation.
- [x] DXGI swap chain.
- [x] Render-target view.
- [x] GPU frame clear and present.
- [x] Command-line test interface.
- [x] Fixed-frame smoke-test mode.

### 0.0.5 — ATLAS

- [x] Runtime HLSL compilation.
- [x] DirectX vertex shader.
- [x] DirectX pixel shader.
- [x] Input layout.
- [x] Vertex buffer.
- [x] Viewport setup.
- [x] First colored GPU triangle.
- [x] `--graphics-test` command.
- [x] Automatic graphics-pipeline smoke test.

### Next graphics work

- [ ] Handle swap-chain resize properly.
- [ ] Move bring-up shaders out of renderer implementation.
- [ ] Common shader abstraction.
- [ ] Common GPU-buffer abstraction.
- [ ] Renderer draw-command API.
- [ ] Texture loading.
- [ ] Basic 2D rendering.
- [ ] Camera system.
- [ ] Vulkan backend.
- [ ] Metal backend.

### Core / engine work

- [ ] Basic keyboard and mouse input API.
- [ ] Cleaner application/game loop separation.
- [ ] Automated CI tests.
- [ ] Asset system.
- [ ] Scene system.
- [ ] Entity/component architecture.
- [ ] Audio layer.
- [ ] Plugin system.
- [ ] Editor prototype.

## Design principles

- Core code should avoid OS-specific types.
- Platform code belongs in `Engine/Platform`.
- Graphics-backend code belongs in `Engine/Renderer`.
- Vendor/store integrations should be optional.
- New architecture should be introduced only when there is something concrete to test with it.
- Unsupported features should fail explicitly instead of pretending to work.

## Versioning

Jevaing is pre-alpha and uses `0.x.x` versions.

Until `1.0`, APIs, source layout and internal architecture may change without backwards-compatibility guarantees.

Development releases use internal codenames such as `RENACO`, `MARIA`, `ARPA`, `ARPA+` and `ATLAS`.

## Contributing

Prefer small, independently testable changes. Keep platform-specific and renderer-specific code isolated behind their respective abstractions whenever possible.

Avoid introducing mandatory vendor services or large dependencies into the core without a clear reason.

## License

Jevaing is licensed under the **MIT License**.

Copyright (c) 2026 jesunixtux.

See [`LICENSE`](LICENSE) for the full license text.
