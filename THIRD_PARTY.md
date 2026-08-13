# Third-party dependencies

Jevaing itself remains MIT licensed.

For external 3D model import, development builds may use **Open Asset Import Library (Assimp) 6.0.5**, licensed under the BSD 3-Clause license. The dependency is kept behind Jevaing's model-import boundary and is not part of the public rendering API.

CMake uses an installed Assimp package when available and otherwise fetches the pinned upstream source release. Refer to the Assimp source distribution for its complete license text and notices.

For PNG/JPG texture decoding, development builds may use **stb_image** from the public-domain/MIT licensed stb project, pinned to commit `2c980bb59875b0d32144a71867fbdebb2f77cd20`. The decoder is kept behind Jevaing's texture-loader boundary and is not part of the public rendering API.

For 2D physics, development builds may use **Box2D 3.1.1**, licensed under the MIT License and pinned to tag `v3.1.1` / commit `8c661469c9507d3ad6fbd2fea3f1aa71669c2fe3`. Jevaing keeps Box2D behind `PhysicsWorld2D` and neutral 2D physics components; client code must not include Box2D headers or store Box2D handles.

For 3D physics, development builds may use **Jolt Physics 5.6.0**, licensed under the MIT License and pinned to tag `v5.6.0` / commit `e77f175595e64cb44218cc9d9d56fc365ad0e36a`. Jevaing keeps Jolt behind `PhysicsWorld3D` and neutral 3D physics components; client code must not include Jolt headers or store Jolt handles.

For the first-party editor UI, development builds may use **Dear ImGui 1.92.1 docking**, licensed under the MIT License and fetched from the official repository `https://github.com/ocornut/imgui.git` pinned to commit `e0b59689461f03cbdc9aa394ea1512c1f0e6e246` (`v1.92.1-docking`). Dear ImGui is linked only by `JevaingEditor` when `JEVAING_BUILD_EDITOR=ON`; normal game executables do not need to link ImGui.

Xbox Dev Mode / UWP and Xbox GDK support do not add proprietary Microsoft SDK files to this repository. Any required Windows SDK, UWP tooling, certificate, package signing material, Xbox Dev Mode deployment tooling, or authorized GDK installation must be provided locally by the developer and kept out of Git.
