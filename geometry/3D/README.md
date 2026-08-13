# Jevaing 3D geometry

This directory contains renderer-neutral 3D geometry data and import code.

## Current layout

- `Mesh.h`: Jevaing's internal triangle-mesh representation.
- `ModelLoader.*`: external-model import boundary.
- `Primitives/Cube.*`: built-in cube geometry used by `DrawCube`.
- `.hide/easter/`: development-only external models used by command-line graphics tests.

## External model tests

From the repository root:

```powershell
.\bin\Debug\JevaingSandbox.exe --penguin-test-3d
.\bin\Debug\JevaingSandbox.exe --gummy3d-test
```

`--penguin-test-3d` loads `.hide/easter/tux.glb`.

`--gummy3d-test` loads `.hide/easter/gummybear.fbx`.

Both assets are centered and normalized for preview, then rotated through the existing Jevaing 3D camera/depth pipeline.

## Import boundary

Jevaing currently uses Assimp 6.0.5 for external model import. CMake first looks for an installed Assimp package; if none is available, it fetches the pinned source release. The importer is isolated here so DirectX, Vulkan and Metal backends do not need to know anything about FBX or glTF parsing.

The current Jevaing mesh path intentionally imports static triangle geometry only. Textures, skeletal animation, bones and model-authored animation are not part of this first external-model test.
