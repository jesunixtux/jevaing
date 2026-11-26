#ifndef JEVAING_MODULE_API_H
#define JEVAING_MODULE_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JEVAING_MODULE_ABI_VERSION 1u

/* Service pointers must refer to versioned C function tables, never C++ objects.
   They remain owned by the host and valid only while the module is loaded. */
typedef struct jevaing_module_host_v1 {
    uint32_t abi_version;
    uint32_t struct_size;
    void* user_data;
    void (*log)(void* user_data, int32_t level, const char* message);
    const void* (*get_service)(void* user_data, const char* name, uint32_t version);
} jevaing_module_host_v1;

typedef struct jevaing_module_v1 {
    uint32_t abi_version;
    uint32_t struct_size;
    void* user_data;
    int32_t (*on_load)(void* user_data, const jevaing_module_host_v1* host);
    void (*on_unload)(void* user_data);
    void (*on_tick)(void* user_data, double delta_time);
} jevaing_module_v1;

/* Export this unmangled symbol from a trusted native module.
   Return zero on success and fill every field in the supplied struct. */
typedef int32_t (*jevaing_module_query_fn)(uint32_t host_abi, jevaing_module_v1* out_module);

#ifdef __cplusplus
}
#endif
#endif
