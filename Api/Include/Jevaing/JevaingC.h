#ifndef JEVAING_C_API_H
#define JEVAING_C_API_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define JEVAING_C_ABI_VERSION 1u
#define JEVAING_C_OK 0
#define JEVAING_C_INVALID_ARGUMENT (-1)
#define JEVAING_C_INVALID_STATE (-2)
#define JEVAING_C_INTERNAL_ERROR (-3)
typedef struct jevaing_vec2 { float x, y; } jevaing_vec2;
typedef struct jevaing_vec3 { float x, y, z; } jevaing_vec3;
typedef struct jevaing_color { float r, g, b, a; } jevaing_color;
typedef struct jevaing_transform { jevaing_vec3 position, rotation, scale; } jevaing_transform;
typedef int32_t jevaing_key;
enum {
    JEVAING_KEY_W=1, JEVAING_KEY_A=2, JEVAING_KEY_S=3, JEVAING_KEY_D=4,
    JEVAING_KEY_UP=5, JEVAING_KEY_DOWN=6, JEVAING_KEY_LEFT=7, JEVAING_KEY_RIGHT=8,
    JEVAING_KEY_SPACE=9, JEVAING_KEY_ENTER=10, JEVAING_KEY_ESCAPE=11
};
typedef struct jevaing_game_desc {
    uint32_t abi_version, struct_size;
    const char* title;
    int32_t width, height;
    void* user_data;
    void (*on_start)(void*);
    void (*on_update)(void*, double);
    void (*on_render_2d)(void*);
    void (*on_render_3d)(void*);
    void (*on_resize)(void*, int32_t, int32_t);
    void (*on_stop)(void*);
} jevaing_game_desc;
#define JEVAING_GAME_DESC_INIT {JEVAING_C_ABI_VERSION, (uint32_t)sizeof(jevaing_game_desc), "Jevaing C", 1280, 720, 0, 0, 0, 0, 0, 0, 0}
uint32_t jevaing_abi_version(void);
const char* jevaing_engine_version(void);
/* Blocking, main thread only. Descriptor and user data must remain valid.
   Negative values are C API errors; nonnegative values are engine exit codes.
   Callbacks must not throw or longjmp across the engine. */
int32_t jevaing_run(const jevaing_game_desc* desc, int argc, char** argv);
/* Input is callback-thread only; returns 0/1 or a negative error.
   Draw functions are valid only during their corresponding render callback. */
int32_t jevaing_key_down(jevaing_key key);
int32_t jevaing_clear(jevaing_color color);
int32_t jevaing_draw_quad(jevaing_vec2 center, jevaing_vec2 size, jevaing_color color);
int32_t jevaing_draw_cube(jevaing_transform transform, jevaing_color color);
#ifdef __cplusplus
}
#endif
#endif
