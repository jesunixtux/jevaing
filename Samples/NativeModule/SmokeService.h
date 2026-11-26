#pragma once
#include <stdint.h>

typedef struct smoke_service_v1 {
    uint32_t version;
    uint32_t struct_size;
    void* user_data;
    void (*record)(void* user_data, int32_t event);
} smoke_service_v1;
