# Experimental C API / API C experimental

`Api/Include/Jevaing/JevaingC.h` exposes ABI version 1. `Samples/MinimalC/main.c` is a pure C11 consumer and `--abi-test` checks linkage and basic argument/version behavior.

Implemented: game lifecycle callbacks, keyboard state, 2D clear/quad and 3D cube drawing. `jevaing_run` blocks on the main thread. The caller owns the descriptor and `user_data`; both must remain valid until it returns. Drawing calls are valid only inside the matching render callback. C callbacks must not unwind across the ABI. The implementation catches C++ exceptions before returning error codes.

Pending: C scene/entity/component handles, physics, audio, assets, ownership APIs and a full C game sample with those features. ABI v1 must not be described as complete C game development support.
