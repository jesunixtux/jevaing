# Build and export / Compilación y exportación

Windows desktop is the only implemented graphical target. Configure with CMake 3.20+ and MSVC:

```powershell
cmake -S . -B build -DJEVAING_BUILD_C_SAMPLE=ON -DJEVAING_BUILD_EDITOR=ON -DJEVAING_BUILD_XBOX_SMOKE_TEST=OFF
cmake --build build --config Debug --target JevaingSandbox JevaingEditor JevaingCSample
cmake --build build --config Release --target JevaingSandbox JevaingEditor JevaingCSample
```

The editor's Build Settings requires explicit trust for the project's CMake scripts. Building a project executes its C/C++ and CMake code with the developer's privileges. Export staging must stay inside the project, outside source, scene, asset and build directories. The export includes the executable, Assets, selected scenes and updated `jevaing.project`. The generated CMake project needs `JEVAING_ENGINE_ROOT` at build time, but the exported game should not need engine source at runtime. This has not been verified on a clean Windows machine.

Options currently include `JEVAING_BUILD_EDITOR`, `JEVAING_BUILD_SANDBOX`, `JEVAING_BUILD_C_SAMPLE`, `JEVAING_ENABLE_PHYSICS_2D`, `JEVAING_ENABLE_PHYSICS_3D` and the experimental Xbox smoke target. Runtime and SDK are not yet separately packaged products.
