# Developer SDK / SDK para desarrolladores

Current SDK surface: public C++ headers in `Api/Include/Jevaing`, experimental `JevaingC.h`/`JevaingModule.h`, and the `JevaingEngine` CMake target. The generated project template builds C and C++ source against this target. The optional `JevaingModules` target provides a Windows native loader. A redistributable, installed SDK package with stable binary interfaces is pending.

Do not ship or link private platform SDKs in this public repository. The module C interface is experimental; external adapter packaging and platform validation remain pending.
