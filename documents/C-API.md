# Experimental C API / API C experimental

`Api/Include/Jevaing/JevaingC.h` exposes ABI version 1. `Samples/MinimalC/main.c` is a pure C11 consumer and `--abi-test` checks linkage and basic argument/version behavior.

Implemented: game lifecycle callbacks, keyboard state, 2D clear/quad/sprite and 3D cube drawing. Opaque `jevaing_scene` handles support create/destroy, save/load, entity create/destroy/count, parent relationships and local transforms. Basic static/kinematic/dynamic box bodies, scene stepping and linear velocity are available for 2D and 3D when the corresponding optional backend is compiled. Call `jevaing_scene_update(scene, 0)` after adding colliders to create bodies before setting velocity. These functions do not yet expose all physics settings or collision events.

The caller owns a scene returned by `jevaing_scene_create` and must call `jevaing_scene_destroy` exactly once. Entity IDs belong to that scene; loading replaces its entities and invalidates previous IDs. Scene operations are not thread-safe. `JEVAING_INVALID_ENTITY_ID` removes a parent. Save/load report `JEVAING_C_IO_ERROR` without a detailed C error string. `jevaing_texture_load` and `jevaing_texture_create_checker` return owned opaque handles; destroy them with `jevaing_texture_destroy`. Keep a texture handle alive through each sprite draw call.

`jevaing_run` blocks on the main thread. The caller owns the descriptor and `user_data`; both must remain valid until it returns. Drawing calls are valid only inside the matching render callback. C callbacks must not unwind across the ABI. The implementation catches C++ exceptions before returning error codes.

Pending: C components beyond transforms and box bodies, direct scene rendering, collision events, model assets, audio and a full C game sample with those features. ABI v1 must not be described as complete C game development support.
