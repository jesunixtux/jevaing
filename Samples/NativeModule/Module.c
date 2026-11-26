#include <Jevaing/JevaingModule.h>
#include "SmokeService.h"

#ifndef MODULE_ID
#error MODULE_ID must be set
#endif

typedef struct module_state { const smoke_service_v1* service; } module_state;
static module_state state = {0};

static int32_t on_load(void* user, const jevaing_module_host_v1* host) {
    module_state* current = (module_state*)user;
    if (!host || host->abi_version != JEVAING_MODULE_ABI_VERSION ||
        host->struct_size != sizeof(*host) || !host->get_service) return -1;
    current->service = (const smoke_service_v1*)host->get_service(
        host->user_data, "SmokeCounter", 1);
    if (!current->service || current->service->version != 1 ||
        current->service->struct_size != sizeof(smoke_service_v1) ||
        !current->service->record) return -1;
    current->service->record(current->service->user_data, MODULE_ID * 10 + 1);
    return 0;
}

static void on_unload(void* user) {
    module_state* current = (module_state*)user;
    if (current->service)
        current->service->record(current->service->user_data, MODULE_ID * 10 + 3);
    current->service = 0;
}

static void on_tick(void* user, double delta_time) {
    module_state* current = (module_state*)user;
    if (delta_time >= 0 && current->service)
        current->service->record(current->service->user_data, MODULE_ID * 10 + 2);
}

__declspec(dllexport) int32_t jevaing_module_query(uint32_t host_abi,
    jevaing_module_v1* out_module) {
    if (host_abi != JEVAING_MODULE_ABI_VERSION || !out_module) return -1;
    out_module->abi_version = JEVAING_MODULE_ABI_VERSION;
    out_module->struct_size = sizeof(*out_module);
    out_module->user_data = &state;
    out_module->on_load = on_load;
    out_module->on_unload = on_unload;
    out_module->on_tick = on_tick;
    return 0;
}
