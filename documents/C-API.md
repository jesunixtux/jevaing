# Experimental C API / API C experimental

`Api/Include/Jevaing/JevaingC.h` exposes ABI version 1. `Samples/MinimalC/main.c` is a pure C11 consumer and `--abi-test` checks linkage and basic argument/version behavior.

Implemented: game lifecycle callbacks, keyboard state, 2D clear/quad and 3D cube drawing. Opaque `jevaing_scene` handles support create/destroy, save/load, entity create/destroy/count, parent relationships and local transforms. The caller owns a scene returned by `jevaing_scene_create` and must call `jevaing_scene_destroy` exactly once. Entity IDs belong to that scene; loading replaces its entities and invalidates previous IDs. Scene operations are not thread-safe. `JEVAING_INVALID_ENTITY_ID` removes a parent. Save/load report `JEVAING_C_IO_ERROR` without a detailed C error string.

`jevaing_run` blocks on the main thread. The caller owns the descriptor and `user_data`; both must remain valid until it returns. Drawing calls are valid only inside the matching render callback. C callbacks must not unwind across the ABI. The implementation catches C++ exceptions before returning error codes.

Pending: C component APIs beyond transforms, scene rendering/updates through the C API, physics, audio, assets and a full C game sample with those features. ABI v1 must not be described as complete C game development support.
