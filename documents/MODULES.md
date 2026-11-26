# Modules / Módulos

Dynamic module loading, versioned plugin interfaces, manifests, dependency resolution and service registration are pending. The current `JevaingEngine` is one static target; optional Box2D/Jolt support is selected at CMake configure time.

Design requirement: native plugins run with the host process's privileges. A manifest can describe compatibility and dependencies, but cannot sandbox a DLL. Proprietary integrations such as Steamworks or licensed physics SDKs must live in separately distributed adapters.
