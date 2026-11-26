# Test status / Estado de pruebas — 2026-11-25

Local Windows MSVC 2022 Debug and Release builds succeeded for `JevaingSandbox`, `JevaingEditor` and `JevaingCSample` in a fresh CMake directory. The existing `build` cache failed because it referenced a removed Visual Studio Build Tools instance; a new `build-codex` configuration succeeded.

Executed successfully in both Debug and Release: `--self-test`, `--project-template-test`, `--scene-serialization-test`, `--editor-scene-roundtrip-test`, `--playmode-restore-test`, `--physics-2d-test`, `--physics-3d-test`, editor `--editor-info`, C sample `--abi-test` and `--windows-build-test`. The export test builds a generated game, runs two frames using the null renderer, selects an alternate startup scene, and checks that an excluded scene is absent. The self-test includes path rejection, build trust, output confinement and backup recovery. Scene serialization also rejects asset traversal.

The pure C sample additionally passed ABI validation, scene save/load roundtrip and two-frame game lifecycle in Debug and Release after the scene/entity ABI extension.

Still unverified: clean-machine runtime dependencies, GUI interaction, real game code in editor Play, Linux/macOS, optional modules/mods/audio/recovery. CI now requests Debug and Release but its result must be observed before claiming a remote pass.
