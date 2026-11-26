# Changelog / Registro de cambios

## Unreleased

- Validate project names and confine project/resource paths to the project directory.
- Save project and scene files through a temporary file, retain a `.bak` copy and recover when the main file is missing.
- Replace shell based CMake invocation with Windows process creation and require explicit trust for project CMake scripts.
- Stage only selected scenes and update exported startup scene metadata.
- Give editor Play snapshots unique names and remove them after loading.
- Run Windows CI checks for Debug and Release configurations.
- Extend experimental C ABI v1 with opaque scene ownership, entity IDs, hierarchy, transforms and scene save/load; test from C11 in both configurations.

No version change. See `documents/TESTING.md` for actual local results and remaining limits.
