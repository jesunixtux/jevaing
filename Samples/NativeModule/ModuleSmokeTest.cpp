#include <Jevaing/ModuleManager.h>
#include "SmokeService.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 2 || !argv[1]) return 2;
    std::vector<std::int32_t> events;
    smoke_service_v1 service{};
    service.version = 1;
    service.struct_size = sizeof(service);
    service.user_data = &events;
    service.record = [](void* user, std::int32_t event)
    { static_cast<std::vector<std::int32_t>*>(user)->push_back(event); };

    Jevaing::ModuleManager manager;
    std::string error;
    if (!manager.RegisterService("SmokeCounter", 1, &service, error) ||
        !manager.Load(argv[1], error) || manager.Count() != 2)
    {
        std::cerr << "Module load failed: " << error << '\n';
        return 2;
    }
    if (manager.RegisterService("SmokeCounter", 1, &service, error) ||
        !manager.Load(argv[1], error) || manager.Count() != 2 || events.size() != 2)
    {
        std::cerr << "Service uniqueness or duplicate module loading failed.\n";
        return 2;
    }
    manager.Tick(1.0 / 60.0);
    manager.UnloadAll();
    const std::array<std::int32_t, 6> expected = {11,21,12,22,23,13};
    if (!std::equal(events.begin(), events.end(), expected.begin(), expected.end()) ||
        manager.Count() != 0)
    {
        std::cerr << "Module lifecycle order is incorrect.\n";
        return 2;
    }
    const auto directory = std::filesystem::path(argv[1]).parent_path();
    for (const char* invalid : {"BadAbi.jevmod", "BadPath.jevmod", "CycleA.jevmod"})
    {
        if (manager.Load((directory / invalid).string(), error) || manager.Count() != 0)
        {
            std::cerr << "Invalid module manifest was accepted: " << invalid << '\n';
            return 2;
        }
    }
    std::cout << "[PASS] Versioned module dependencies, service lookup and lifecycle.\n";
    return 0;
}
