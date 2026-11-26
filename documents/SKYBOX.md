# Procedural skybox / Cielo procedural

`Jevaing::Skybox` defines zenith, horizon and nadir colors. A scene can enable it with `scene.SetSkybox(skybox)`, disable it with `scene.ClearSkybox()`, and save it to a `.scene` file. The editor exposes these three colors under **Inspector → Scene Settings** when no entity is selected.

The Windows DirectX 11 renderer draws the sky gradient behind 3D geometry. The horizon shifts when the camera looks up or down; camera translation does not move the background. This first implementation has no cubemap textures, sun, clouds, atmospheric scattering or image based lighting. The null renderer accepts the call without drawing pixels. The 2D renderer does not use this skybox; 2D games draw their own backgrounds.

Scene file example:

```ini
scene=Example
skybox.zenith=0.04,0.12,0.33,1
skybox.horizon=0.48,0.7,0.94,1
skybox.nadir=0.1,0.14,0.23,1
```

`Samples/Minimal3D/Scenes/main.scene` enables it. Omit all three `skybox.*` keys for the existing clear color. Colors are stored as RGBA; the renderer uses RGB and treats the background as opaque.
