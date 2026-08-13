# Jevaing

Jevaing is an experimental open-source graphics and game engine written in C++17 and built with CMake.

> **Current version:** `0.0.10`
>
> **Codename:** `TBD`
>
> **Status:** early prototype, not production ready.

## What 0.0.10 Changes

Jevaing 0.0.10 adds the first backend-neutral rigid-body physics foundation while keeping SDK details out of gameplay code.

The public Scene/Entity model now supports:

- `RigidBody2DComponent`
- `BoxCollider2DComponent`
- `CircleCollider2DComponent`
- `RigidBody3DComponent`
- `BoxCollider3DComponent`
- `SphereCollider3DComponent`
- `CapsuleCollider3DComponent`
- `PhysicsMaterial`
- `PhysicsWorld2D`
- `PhysicsWorld3D`
- collision and trigger enter/exit events
- neutral `Raycast2D` / `Raycast3D` results
- fixed timestep simulation at `1.0 / 60.0` by default

Backends are selected through neutral enums:

```cpp
Jevaing::Physics2DBackend::Box2D
Jevaing::Physics3DBackend::Jolt
```

Box2D and Jolt headers stay private to `Engine/Physics`. `Api/Include/Jevaing` does not include Box2D, Jolt, DirectX or Win32 headers.

## External Physics Sample

`Samples/Physics3D` is the 0.0.10 proof that physics works outside `JevaingSandbox`.

```powershell
cmake -S Samples\Physics3D -B build-physics3d
cmake --build build-physics3d --config Debug
.\build-physics3d\bin\Debug\Physics3D.exe --frames 120
```

The sample has its own `CMakeLists.txt`, `jevaing.project`, `Scenes/main.scene` and `Source/Game.cpp`. It includes only:

```cpp
#include <Jevaing/Jevaing.h>
```

It demonstrates gravity, static ground, dynamic cube/sphere bodies, a small box stack, trigger overlap, raycast and impulse via the public API.

## What 0.0.9 Changed

Jevaing 0.0.9 moves the engine from internal demos toward real external prototypes.

The main proof is `Samples/Minimal3D`: it has its own `CMakeLists.txt`, `jevaing.project`, `Scenes/main.scene`, assets, and `Source/Game.cpp`. It builds a standalone `Minimal3D.exe` without modifying `Sandbox/main.cpp`.

Conceptually:

```text
Project
    -> Scene
    -> Entity + Components
    -> Assets / Input
    -> Graphics2D / Graphics3D
    -> Renderer
    -> Game executable
```

## Creating Your First Jevaing Prototype

Use the Minimal3D sample as the template:

```powershell
cmake -S Samples\Minimal3D -B build-minimal3d
cmake --build build-minimal3d --config Debug
.\build-minimal3d\bin\Debug\Minimal3D.exe
```

`Samples/Minimal3D/Source/Game.cpp` includes only:

```cpp
#include <Jevaing/Jevaing.h>
```

and links with:

```cmake
target_link_libraries(Minimal3D PRIVATE JevaingEngine)
```

It does not include Win32, DirectX, Assimp, stb, or engine-private headers.

## Project System

Project files use a small `key=value` format:

```text
name=Minimal3D
startupScene=Scenes/main.scene
assetRoot=Assets
sceneRoot=Scenes
```

Public API:

```cpp
Jevaing::ProjectConfig config;
std::string error;
Jevaing::Project::Load("jevaing.project", config, error);
```

`Project::ResolveStartupScenePath` and `Project::ResolveAssetPath` resolve project-relative paths without personal absolute paths.

## Scene Format

Scenes use a simple text format in 0.0.9:

```text
scene=Minimal3D

[entity]
id=1
name=Camera
parent=0
position=0,1,-5
rotation=0,0,0
scale=1,1,1
camera.primary=1
camera.fov=1.04719755
camera.near=0.1
camera.far=100
[/entity]

[entity]
id=2
name=Tux
parent=0
position=0,0,0
rotation=0,0,0
scale=1,1,1
mesh.model=Models/tux.glb
[/entity]
```

The scene stores IDs, names, local transforms, parent relationships, camera data, mesh references, material color overrides, and sprite references. It stores asset paths, not vertex data or GPU resources.

## Scene And Entity

Public scene API includes:

- `Scene::CreateEntity`
- `Scene::CreateEntityWithId`
- `Scene::DestroyEntity`
- `Scene::FindEntity`
- `Scene::FindEntityByName`
- `Scene::SetParent`
- `Scene::RemoveParent`
- `Scene::GetWorldTransform`
- `Scene::LoadFromFile`
- `Scene::Save`
- `Scene::Update`
- `Scene::Render`

`EntityId` is a serializable `uint64_t`. `0` is reserved as `InvalidEntityId`.

## Components

Components remain small and value-oriented:

- `TransformComponent`
- `CameraComponent`
- `MeshRendererComponent`
- `SpriteRenderer2DComponent`
- `RigidBody2DComponent`
- `BoxCollider2DComponent`
- `CircleCollider2DComponent`
- `RigidBody3DComponent`
- `BoxCollider3DComponent`
- `SphereCollider3DComponent`
- `CapsuleCollider3DComponent`

There is no full ECS, reflection system, scripting, prefab system or editor yet.

## Physics

Each `Scene` owns its own `PhysicsWorld2D` and `PhysicsWorld3D`. There is no global physics world.

Transform ownership in 0.0.10 is explicit:

```text
Static/Kinematic: Scene Transform -> Physics
Dynamic:          Physics -> Scene Transform
```

The update order is:

```text
1. Sync Static/Kinematic Scene transforms into Physics
2. Step Physics with a fixed timestep accumulator
3. Sync Dynamic Physics transforms back into Scene
4. Render
```

For this MVP, Dynamic rigid bodies under a parent entity are rejected cleanly. This avoids silently building physics on top of the current simplified hierarchy conversion. The restriction is covered by `--physics-hierarchy-test`.

## Transform Hierarchy

Entities can be parented:

```text
Parent
    -> Child
        -> GrandChild
```

`Scene::SetParent` rejects missing entities, self-parenting, and cycles. `GetWorldTransform` combines parent transforms so children follow parent movement/rotation/scale.

## Rendering

0.0.9 keeps the public rendering API backend-neutral:

- `Graphics3D::DrawMesh`
- `Graphics3D::DrawCube`
- `Graphics3D::SetDirectionalLight`
- `Graphics2D::DrawSprite`
- `Graphics2D::DrawQuad`
- `Graphics2D::DrawTriangle`

`MeshRendererComponent` renders imported assets from 0.0.8. `SpriteRenderer2DComponent` renders a `Texture2D` through `DrawSprite`.

## GPU Mesh Resources

DirectX 11 now creates persistent renderer-side mesh resources:

```text
CPU Mesh
    -> D3D11 vertex buffer
    -> D3D11 index buffer
    -> DrawIndexed
```

The public API still sees `Jevaing::Mesh`; D3D11 buffers are private to the renderer. `--gpu-mesh-test` renders the same mesh repeatedly and checks that buffers are reused instead of recreated every frame.

Texture SRVs are now cached by a stable texture key derived from source path, size and format, not by raw CPU object address.

## 0.0.8 Debt Fixed

- Imported vertices now fall back to white vertex color when material color owns the real color, avoiding `BaseColor * BaseColor`.
- DirectX uses a normal matrix path for non-uniform scale instead of transforming normals directly by the full model matrix.
- Texture GPU cache no longer keys by raw `Texture2D*`.
- Asset cache lowercases paths only on Windows.

## Input

Mouse input is public and backend-neutral:

- `Input::GetMousePosition`
- `Input::GetMouseDelta`
- `Input::GetMouseWheelDelta`
- `Input::IsMouseButtonDown`
- `Input::IsMouseButtonPressed`
- `Input::IsMouseButtonReleased`

`InputMap` is a tiny action layer:

```cpp
Jevaing::InputMap input;
input.Bind("MoveForward", Jevaing::Key::W);
input.Bind("Boost", Jevaing::MouseButton::Left);
```

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
.\bin\Debug\JevaingSandbox.exe --model-test "geometry\3D\.hide\easter\tux.glb"
.\bin\Debug\JevaingSandbox.exe --texture-test
.\bin\Debug\JevaingSandbox.exe --material-test
.\bin\Debug\JevaingSandbox.exe --lighting-test
.\bin\Debug\JevaingSandbox.exe --multi-model-test
.\bin\Debug\JevaingSandbox.exe --asset-cache-test
.\bin\Debug\JevaingSandbox.exe --asset-error-test
.\bin\Debug\JevaingSandbox.exe --asset-info "geometry\3D\.hide\easter\tux.glb"
.\bin\Debug\JevaingSandbox.exe --mixed-2d-3d-test
```

New 0.0.9 tests:

```powershell
.\bin\Debug\JevaingSandbox.exe --scene-test
.\bin\Debug\JevaingSandbox.exe --scene-serialization-test
.\bin\Debug\JevaingSandbox.exe --hierarchy-test
.\bin\Debug\JevaingSandbox.exe --mouse-test
.\bin\Debug\JevaingSandbox.exe --sprite-test
.\bin\Debug\JevaingSandbox.exe --gpu-mesh-test
.\bin\Debug\JevaingSandbox.exe --project-test "Samples\Minimal3D\jevaing.project"
```

New 0.0.10 physics tests:

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

Visual tests should be inspected by a person. A passing exit code means the runtime initialized, ran and shut down cleanly.

## Windows/MSVC Validation

```powershell
git pull origin main

cmake -S . -B build
cmake --build build --config Debug --clean-first

.\bin\Debug\JevaingSandbox.exe --version
.\bin\Debug\JevaingSandbox.exe --self-test
.\bin\Debug\JevaingSandbox.exe --runtime-test
.\bin\Debug\JevaingSandbox.exe --asset-cache-test
.\bin\Debug\JevaingSandbox.exe --asset-error-test

.\bin\Debug\JevaingSandbox.exe --scene-test
.\bin\Debug\JevaingSandbox.exe --scene-serialization-test
.\bin\Debug\JevaingSandbox.exe --hierarchy-test
.\bin\Debug\JevaingSandbox.exe --sprite-test
.\bin\Debug\JevaingSandbox.exe --gpu-mesh-test
.\bin\Debug\JevaingSandbox.exe --project-test "Samples\Minimal3D\jevaing.project"
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

cmake -S Samples\Minimal3D -B build-minimal3d
cmake --build build-minimal3d --config Debug
.\build-minimal3d\bin\Debug\Minimal3D.exe --frames 180

cmake -S Samples\Physics3D -B build-physics3d
cmake --build build-physics3d --config Debug
.\build-physics3d\bin\Debug\Physics3D.exe --frames 120
```

## Repository Structure

```text
jevaing/
|-- Api/Include/Jevaing/
|   |-- Assets.h
|   |-- Components.h
|   |-- Entity.h
|   |-- Game.h
|   |-- Graphics2D.h
|   |-- Graphics3D.h
|   |-- Input.h
|   |-- Jevaing.h
|   |-- Physics.h
|   |-- Project.h
|   |-- Scene.h
|   |-- Types.h
|-- Engine/
|   |-- Physics/
|-- geometry/
|-- Sandbox/
|-- Samples/Minimal3D/
|-- Samples/Physics3D/
```

## Current Limitations

- Physics2D sample was not added in 0.0.10; automated 2D physics tests were prioritized.
- Scene format is intentionally simple text, not full JSON.
- Dynamic physics bodies cannot be parented in 0.0.10.
- Collision response is intentionally MVP-level and focused on box/sphere/circle smoke coverage.
- No editor, prefab system, scripting, audio, networking, particles or gamepad.
- No skeletal animation, PBR, shadows, Vulkan, Metal, Linux or macOS backend.
- GPU mesh cache is a first DirectX implementation and may need stronger asset lifetime handles later.

## License

Jevaing is licensed under the MIT License.
