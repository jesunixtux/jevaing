# Contributing / Contribuir

Use CMake 3.20 or newer and a Windows MSVC toolchain for the currently supported editor and graphics path. Keep C++ public headers under `Api/Include/Jevaing` and C ABI exports in `JevaingC.h`/`JevaingC.cpp`. Preserve ABI version 1 until incompatible changes are deliberately versioned.

Before proposing changes, build Debug and Release, run the headless sandbox tests and the C ABI sample, and document any tests that could not run. Do not claim platform support based solely on compilation. Keep proprietary SDKs and credentials out of the repository. The project license is in `LICENSE`.
