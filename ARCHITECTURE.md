# Jevaing architecture / Arquitectura

Status: current implementation, 2026-11-25. Jevaing remains an early prototype.

| Area | Current code | Status |
| --- | --- | --- |
| Public C++ API | `Api/Include/Jevaing` | Implemented, C++17 |
| C ABI | `JevaingC.h`, `JevaingC.cpp` | Experimental v1; lifecycle, input, drawing, basic scenes/entities |
| Core and runtime | `Engine/Core` | Implemented in one static library |
| Rendering | `Engine/Renderer`, DirectX 11 | Windows only; null renderer for tests |
| Physics | `Engine/Physics` | Box2D and Jolt optional CMake features |
| Editor | `Editor/Application` | Windows ImGui prototype; scene copy during Play |
| Build tooling | `Engine/Build` | Windows project build/export; requires trusted CMake scripts |
| Audio, scripting, modules, mods | — | Pending |

`JevaingEngine` currently combines runtime, renderer, physics, assets and build tooling. Split these libraries only as working interfaces and tests are added. Proprietary SDKs belong in external adapters, never in this repository.

Exported games use the generated CMake project and the engine source at **build time**. The executable does not require the editor or engine source at runtime; portability to a clean Windows machine still requires validation, including runtime dependencies.
