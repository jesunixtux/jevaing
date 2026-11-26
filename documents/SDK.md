# Developer SDK / SDK para desarrolladores

Current SDK surface: public C++ headers in `Api/Include/Jevaing`, experimental `JevaingC.h` and the `JevaingEngine` CMake target. The generated project template builds C and C++ source against this target. A redistributable, installed SDK package with stable binary interfaces is pending.

Do not ship or link private platform SDKs in this public repository. Versioned plugin interfaces and external adapter packages are planned but not implemented.
