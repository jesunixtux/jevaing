# Jevaing

Jevaing is an experimental open-source graphics and game engine written in C++17 and built with CMake.

> **Current version:** `0.0.8`
>
> **Codename:** `TBD`
>
> **Status:** early prototype, not production ready.

## What 0.0.8 changes

Jevaing 0.0.8 turns the first 3D model experiments into a reusable asset foundation.

The intended path is now:

```text
external file
    -> AssetManager
    -> Model / Mesh / Texture2D / Material
    -> Graphics3D
    -> Renderer
    -> DirectX 11
```

DirectX 11 does not load GLB, FBX, PNG or JPG files. Assimp and stb_image are kept behind Jevaing loader boundaries, and the renderer receives backend-neutral Jevaing data.

New 0.0.8 foundations:

- `Jevaing::Mesh` with vertices, indices, normals, UV0, vertex color and bounds.
- `Jevaing::Vertex3D`.
- `Jevaing::Texture2D` with RGBA8 pixel data.
- `Jevaing::Material` with base color and optional base-color texture.
- `Jevaing::Model` with multiple meshes and materials.
- `Jevaing::DirectionalLight`.
- `Jevaing::Assets::LoadModel`.
- `Jevaing::Assets::LoadTexture2D`.
- `Jevaing::Assets::CreateCheckerTexture`.
- `Graphics3D::DrawMesh`.
- `Graphics3D::SetDirectionalLight`.
- `DrawCube` remains available and is now backed by Jevaing mesh primitives.

Supported model formats in this prototype:

- `.glb`
- `.gltf`
- `.fbx`

Supported texture file formats:

- `.png`
- `.jpg`
- `.jpeg`

## Asset System

`AssetManager` caches loaded models and textures by normalized path. Loading the same path repeatedly while the asset is still referenced returns the cached object instead of reimporting the file.

Assets use shared ownership:

```cpp
std::string error;
auto model = Jevaing::Assets::LoadModel(
    "geometry/3D/.hide/easter/tux.glb",
    &error
);
```

When all `shared_ptr` references are released, the cached weak reference can expire and the asset may be imported again on a later request. This keeps the 0.0.8 cache small and RAII-friendly without a global lifetime trap.

## Graphics3D

The public 3D API now supports backend-neutral mesh drawing:

```cpp
Jevaing::Material material;
material.BaseColor = { 0.25f, 0.72f, 1.0f, 1.0f };

Jevaing::Transform transform;
graphics.DrawMesh(mesh, transform, material);
```

Directional lighting is intentionally simple Lambert/diffuse lighting:

```cpp
Jevaing::DirectionalLight light;
light.Direction = { -0.35f, -0.85f, 0.40f };
light.Color = { 1.0f, 0.96f, 0.88f, 1.0f };
light.Intensity = 1.0f;

graphics.SetDirectionalLight(light);
```

This is not a PBR renderer. It exists to prove that normals survive import and reach the shader.

## Command-line Tests

Existing tests remain available:

```powershell
.\bin\Debug\JevaingSandbox.exe --version
.\bin\Debug\JevaingSandbox.exe --self-test
.\bin\Debug\JevaingSandbox.exe --runtime-test
.\bin\Debug\JevaingSandbox.exe --renderer-info
.\bin\Debug\JevaingSandbox.exe --graphics-test
.\bin\Debug\JevaingSandbox.exe --graphics-test-penguin
.\bin\Debug\JevaingSandbox.exe --graphics-test-3d
.\bin\Debug\JevaingSandbox.exe --penguin-test-3d
.\bin\Debug\JevaingSandbox.exe --gummy3d-test
```

New 0.0.8 tests:

```powershell
.\bin\Debug\JevaingSandbox.exe --model-test "geometry\3D\.hide\easter\tux.glb"
.\bin\Debug\JevaingSandbox.exe --model-test "geometry\3D\.hide\easter\gummybear.fbx"
.\bin\Debug\JevaingSandbox.exe --texture-test
.\bin\Debug\JevaingSandbox.exe --material-test
.\bin\Debug\JevaingSandbox.exe --lighting-test
.\bin\Debug\JevaingSandbox.exe --multi-model-test
.\bin\Debug\JevaingSandbox.exe --asset-cache-test
.\bin\Debug\JevaingSandbox.exe --asset-error-test
.\bin\Debug\JevaingSandbox.exe --asset-info "geometry\3D\.hide\easter\tux.glb"
.\bin\Debug\JevaingSandbox.exe --mixed-2d-3d-test
```

Aliases:

```text
--penguin-test-3d = --model-test geometry/3D/.hide/easter/tux.glb
--gummy3d-test    = --model-test geometry/3D/.hide/easter/gummybear.fbx
```

`--asset-info` is headless and prints available model data such as path, format, mesh count, vertices, indices, triangles, material count, texture references, normals, UV0 and bounds.

## Interactive Sandbox

Build and run:

```powershell
git clone https://github.com/jesunixtux/jevaing.git
cd jevaing
cmake -S . -B build
cmake --build build --config Debug
.\bin\Debug\JevaingSandbox.exe
```

The default Sandbox opens:

```text
Jevaing 0.0.8 - TBD Sandbox
```

Controls:

| Input | Action |
|---|---|
| `WASD` | Move the cube |
| Arrow keys | Rotate the cube |
| `Space` | Change the cube color while held |
| `ESC` | Close the window |

The Sandbox remains a public-API client. It does not include Win32 or DirectX headers.

## Public API Snapshot

The public client API currently includes:

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
Jevaing::Mesh
Jevaing::Vertex3D
Jevaing::Model
Jevaing::Material
Jevaing::Texture2D
Jevaing::DirectionalLight
Jevaing::Assets
Jevaing::Key
Jevaing::Input
Jevaing::Run(...)
Jevaing::GetVersion()
Jevaing::GetCodename()
```

These APIs may change while Jevaing is below version 1.0.

## Repository Structure

```text
jevaing/
|-- Api/
|   |-- Include/Jevaing/
|       |-- Assets.h
|       |-- Game.h
|       |-- Graphics2D.h
|       |-- Graphics3D.h
|       |-- Input.h
|       |-- Jevaing.h
|       |-- Types.h
|
|-- Engine/
|   |-- Core/
|   |   |-- Application.*
|   |   |-- AssetManager.cpp
|   |   |-- CommandLine.*
|   |   |-- Input.*
|   |   |-- Logger.*
|   |   |-- Timer.*
|   |   |-- Version.*
|   |   |-- Window.*
|   |
|   |-- Platform/Windows/
|   |-- Renderer/
|       |-- Renderer.*
|       |-- DirectX/D3D11Renderer.*
|
|-- geometry/
|   |-- 3D/
|       |-- Mesh.h
|       |-- ModelLoader.*
|       |-- TextureLoader.*
|       |-- Primitives/Cube.*
|       |-- .hide/easter/
|
|-- Sandbox/
|-- CMakeLists.txt
|-- README.md
|-- THIRD_PARTY.md
```

## Dependencies

Required on Windows:

- CMake 3.20 or newer.
- C++17-compatible compiler.
- Windows SDK.
- MSVC / Visual Studio Build Tools or another compatible Windows C++ toolchain.

Third-party dependencies:

- Assimp 6.0.5 for model import.
- stb_image pinned by commit for PNG/JPG decoding.

CMake first looks for an installed Assimp package and otherwise fetches the pinned Assimp release. stb is fetched from a pinned upstream commit. Neither dependency is exposed through the DirectX renderer API.

## Current Limitations

0.0.8 is a static asset/rendering foundation, not a complete engine.

Not implemented yet:

- skeletal animation, bones, skinning or animation graphs;
- PBR materials, tangents, bitangents, roughness/metallic pipeline;
- embedded GLB texture extraction;
- missing external model texture packaging;
- shadow maps or advanced lighting;
- sprite/font/text rendering;
- scene system, ECS or editor;
- physics, scripting, audio or networking;
- Vulkan, Metal, Linux or macOS backends.

If a model references an external texture file that is not present next to the model, Jevaing reports the texture load error and renders the material without that texture. The current `gummybear.fbx` references `temp.fbm/texture_0.png`, which is not present in the repository.

## Windows/MSVC Validation Battery

Use this exact battery from a normal checkout:

```powershell
git pull origin main

cmake -S . -B build
cmake --build build --config Debug --clean-first

.\bin\Debug\JevaingSandbox.exe --version
.\bin\Debug\JevaingSandbox.exe --self-test
.\bin\Debug\JevaingSandbox.exe --runtime-test
.\bin\Debug\JevaingSandbox.exe --renderer-info

.\bin\Debug\JevaingSandbox.exe --graphics-test
.\bin\Debug\JevaingSandbox.exe --graphics-test-penguin
.\bin\Debug\JevaingSandbox.exe --graphics-test-3d

.\bin\Debug\JevaingSandbox.exe --penguin-test-3d
.\bin\Debug\JevaingSandbox.exe --gummy3d-test

.\bin\Debug\JevaingSandbox.exe --model-test "geometry\3D\.hide\easter\tux.glb"
.\bin\Debug\JevaingSandbox.exe --model-test "geometry\3D\.hide\easter\gummybear.fbx"

.\bin\Debug\JevaingSandbox.exe --texture-test
.\bin\Debug\JevaingSandbox.exe --material-test
.\bin\Debug\JevaingSandbox.exe --lighting-test
.\bin\Debug\JevaingSandbox.exe --multi-model-test

.\bin\Debug\JevaingSandbox.exe --asset-cache-test
.\bin\Debug\JevaingSandbox.exe --asset-error-test

.\bin\Debug\JevaingSandbox.exe --asset-info "geometry\3D\.hide\easter\tux.glb"

.\bin\Debug\JevaingSandbox.exe --mixed-2d-3d-test
```

Visual tests should be inspected by a person. A passing exit code means the engine initialized, ran the frame loop and shut down cleanly; it does not prove the image looked correct.

## Roadmap

Completed foundations:

- 0.0.1: CMake/C++ core and Win32 window.
- 0.0.2: window abstraction, logger, timer and delta time.
- 0.0.3: renderer abstraction and Null Renderer.
- 0.0.4: DirectX 11 device/swap chain and CLI smoke tests.
- 0.0.5: runtime HLSL shaders and first GPU triangle.
- 0.0.6: public `Game`, input and `Graphics2D`.
- 0.0.7: `Graphics3D`, math types, depth buffer and `DrawCube`.
- 0.0.8: asset, mesh, material, texture and generic model test foundation.

Next useful foundations:

- Mouse input.
- Gamepad input.
- Sprite and text drawing.
- 2D camera/world coordinates.
- Stronger GPU mesh/resource lifetime model.
- Scene foundation.
- Audio layer.
- Vulkan backend.
- Metal backend.
- Automated CI on supported platforms.

## Design Principles

- Core/client code should not require OS-specific types.
- Platform code belongs in `Engine/Platform`.
- Renderer backend code belongs in `Engine/Renderer`.
- Asset importers belong behind neutral Jevaing data types.
- DirectX, Vulkan and Metal should not know how to parse external asset formats.
- Unsupported features should fail explicitly.
- New systems should come with concrete CLI validation.

## License

Jevaing is licensed under the MIT License.

Copyright (c) 2026 jesunixtux.

See [`LICENSE`](LICENSE) for the full license text.
