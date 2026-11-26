# Jevaing public API

The `Api/Include/Jevaing` directory contains the headers intended for client code.

As of Jevaing 0.0.14 TBD the public surface includes:

- `Assets.h` - backend-neutral asset data types and loading helpers for `Model`, `Mesh`, `Texture2D` and `Material`.
- `Components.h` - small scene component value types.
- `Entity.h` - serializable `EntityId`.
- `Jevaing.h` - engine version helpers and `Run(...)` entry points.
- `Game.h` - client lifecycle and `GameConfig`.
- `Graphics2D.h` - backend-neutral immediate 2D drawing interface including `DrawSprite`.
- `Graphics3D.h` - backend-neutral immediate 3D drawing interface with `DrawCube`, `DrawMesh` and `SetDirectionalLight`.
- `Input.h` - keyboard, mouse, neutral gamepad and small `InputMap` action helpers.
- `Physics.h` - backend-neutral physics enums, components, worlds, events and raycasts.
- `Project.h` - first project config loader.
- `Scene.h` - first scene/entity/component runtime.
- `Types.h` - small public value types such as `Vec2`, `Vec3`, `Mat4`, `Transform`, `PerspectiveCamera` and `Color`.

Client code should prefer these headers instead of including files from `Engine/Core`, `Engine/Platform` or `Engine/Renderer` directly.

Renderer, import, editor, platform input and physics backends stay outside the public API. Client code should not include Dear ImGui, DirectX, Win32, XInput, UWP, GDK, Assimp, stb, Box2D or Jolt headers to use Jevaing assets, input and physics.

Gamepad clients use neutral values only:

- `GamepadButton`
- `GamepadState`
- `Input::IsGamepadConnected`
- `Input::IsGamepadButtonDown`
- `Input::IsGamepadButtonPressed`
- `Input::IsGamepadButtonReleased`
- `Input::GetGamepadState`

Physics clients use neutral values only:

- `BodyType`
- `PhysicsMaterial`
- `RigidBody2DComponent`
- `RigidBody3DComponent`
- `BoxCollider2DComponent`
- `CircleCollider2DComponent`
- `BoxCollider3DComponent`
- `SphereCollider3DComponent`
- `CapsuleCollider3DComponent`
- `PhysicsWorld2D`
- `PhysicsWorld3D`
- `CollisionEvent2D`
- `CollisionEvent3D`
- `RaycastHit2D`
- `RaycastHit3D`

The API is pre-1.0 and may change while Jevaing is still establishing its runtime, asset and scene foundations.

## Experimental C API v1
Include `<Jevaing/JevaingC.h>` in C11 code and link JevaingEngine with the C++ linker. The C API wraps the existing C++ runtime and provides lifecycle callbacks, keyboard input, 2D clear/quad and 3D cube drawing. No C++ ownership crosses the interface. User data belongs to the caller; drawing is render-callback-only. Single active C runtime; call from the main thread. This is a static experimental ABI, not a stable DLL/plugin SDK. Scene, asset and physics C bindings are future work.

Configure Windows with `-DJEVAING_BUILD_C_SAMPLE=ON`, build `JevaingCSample`, then run `build/bin/Debug/JevaingCSample.exe --abi-test`. In a Windows desktop session, run `--runtime-test --renderer null --frames 5` for lifecycle validation (no GPU output), or launch normally for visual validation.
