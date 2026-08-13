# Jevaing public API

The `Api/Include/Jevaing` directory contains the headers intended for client code.

As of Jevaing 0.0.8 TBD the public surface includes:

- `Assets.h` - backend-neutral asset data types and loading helpers for `Model`, `Mesh`, `Texture2D` and `Material`.
- `Jevaing.h` - engine version helpers and `Run(...)` entry points.
- `Game.h` - client lifecycle and `GameConfig`.
- `Graphics2D.h` - backend-neutral immediate 2D drawing interface.
- `Graphics3D.h` - backend-neutral immediate 3D drawing interface with `DrawCube`, `DrawMesh` and `SetDirectionalLight`.
- `Input.h` - basic keyboard key/state queries.
- `Types.h` - small public value types such as `Vec2`, `Vec3`, `Mat4`, `Transform`, `PerspectiveCamera` and `Color`.

Client code should prefer these headers instead of including files from `Engine/Core`, `Engine/Platform` or `Engine/Renderer` directly.

Renderer backends and import libraries stay outside the public API. Client code should not include DirectX, Win32, Assimp or stb headers to use Jevaing assets.

The API is pre-1.0 and may change while Jevaing is still establishing its runtime, asset and scene foundations.
