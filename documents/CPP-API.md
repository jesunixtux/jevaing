# C++ API / API C++

The implemented C++17 public headers are under `Api/Include/Jevaing`. Games implement `Jevaing::Game` and call `Jevaing::Run`. `Scene`, entities/components, input, graphics, assets, project configuration and neutral 2D/3D physics interfaces are available. See `Samples/Minimal3D` and `Samples/Physics3D` for source examples.

Scenes can configure a procedural 3D sky with `Scene::SetSkybox`, `Scene::ClearSkybox` and `Graphics3D::DrawSkybox`; see `documents/SKYBOX.md`. The current engine library is static. Binary compatibility of the C++ API across separately compiled engine versions is not guaranteed. The C ABI is a separate experimental interface. Audio and Lua are pending; trusted native modules are opt-in and experimental.
