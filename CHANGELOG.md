# Changelog / Registro de cambios

## Unreleased

- Add a serializable procedural 3D skybox with DirectX 11 background rendering and editor color controls.
- Add the small playable `SkyDodge2D` game sample and deterministic gameplay logic check.
- Validate project names and confine project/resource paths to the project directory.
- Save project and scene files through a temporary file, retain a `.bak` copy and recover when the main file is missing or invalid. Reject truncated scene entities.
- Replace shell based CMake invocation with Windows process creation and require explicit trust for project CMake scripts.
- Stage only selected scenes and update exported startup scene metadata.
- Give editor Play snapshots unique names and remove them after loading.
- Run Windows CI checks for Debug and Release configurations.
- Extend experimental C ABI v1 with opaque scene ownership, entity IDs, hierarchy, transforms and scene save/load; test from C11 in both configurations.
- Add basic 2D/3D box physics and texture/sprite resource handles to the C ABI, with pure C tests in Debug and Release.
- Add an opt-in Windows native module loader with versioned C interfaces, dependency manifests, a service lookup table and DLL lifecycle tests.
- Add an opt-in content mod resolver with game/version manifests, dependency checks, asset/map policies and non-destructive overlay lookup.

No version change. See `documents/TESTING.md` for actual local results and remaining limits.
