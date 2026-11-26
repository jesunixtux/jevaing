# Modules / Módulos

An experimental Windows native module loader is available with `-DJEVAING_ENABLE_NATIVE_MODULES=ON`. Link `JevaingModules` and include `Jevaing/ModuleManager.h`. Modules are trusted DLLs exporting `jevaing_module_query` with the C structs in `JevaingModule.h`. A `.jevmod` manifest requires `name`, semantic `version`, `abi=1`, and a relative `library` DLL path. Repeat `dependency=Name@major` to require another manifest in the same directory. Loading resolves dependencies first, rejects cycles/incompatible ABI or major versions, and unloads in reverse order. `RegisterService` exposes versioned C function tables through host lookup; the caller owns their lifetime.

`Samples/NativeModule` builds two DLLs and tests service lookup, lifecycle order and invalid manifests. The loader is a separate static target; games that do not link it need not include this infrastructure. Hot reload, module editor UI, signatures, isolation and non-Windows implementations are pending. Optional Box2D/Jolt support is still selected at CMake configure time.

Native plugins run with the host process's privileges. A manifest describes compatibility and dependencies, but cannot sandbox a DLL. Proprietary integrations such as Steamworks or licensed physics SDKs must live in separately distributed adapters.
