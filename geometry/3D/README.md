# Jevaing 3D Geometry

This directory contains renderer-neutral 3D geometry and import code.

## Current Layout

- `Mesh.h`: compatibility aliases to the public Jevaing mesh/model/material/texture types.
- `ModelLoader.*`: Assimp-backed model import boundary.
- `TextureLoader.*`: stb_image-backed texture import boundary for PNG/JPG/JPEG.
- `Primitives/Cube.*`: built-in indexed cube and plane meshes used by `DrawCube` and CLI tests.
- `.hide/easter/`: development-only external models used by command-line graphics tests.

## Import Boundary

Assimp is used only here to import `.glb`, `.gltf` and `.fbx`. Imported data is converted immediately into Jevaing-owned `Model`, `Mesh`, `Material`, `Vertex3D` and `Texture2D` data before it reaches the renderer.

DirectX 11 does not include Assimp headers and does not know how to load external model formats.

## Tests

From the repository root:

```powershell
.\bin\Debug\JevaingSandbox.exe --model-test "geometry\3D\.hide\easter\tux.glb"
.\bin\Debug\JevaingSandbox.exe --model-test "geometry\3D\.hide\easter\gummybear.fbx"
.\bin\Debug\JevaingSandbox.exe --multi-model-test
.\bin\Debug\JevaingSandbox.exe --asset-info "geometry\3D\.hide\easter\tux.glb"
```

Compatibility aliases:

```powershell
.\bin\Debug\JevaingSandbox.exe --penguin-test-3d
.\bin\Debug\JevaingSandbox.exe --gummy3d-test
```

Both aliases now use the same generic asset/model path as `--model-test`.

## Limitations

The current loader imports static triangle geometry, materials with base color, UV0 and normals. It can preserve texture references and load external PNG/JPG/JPEG files when those files exist on disk.

Embedded GLB texture extraction, skeletal animation, skinning, morph targets and PBR material channels are not implemented in 0.0.8.
