#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace Jevaing
{
    // Main-thread, trusted-code loader. Build and link JevaingModules to use it.
    class ModuleManager
    {
    public:
        ModuleManager();
        ~ModuleManager();
        ModuleManager(const ModuleManager&) = delete;
        ModuleManager& operator=(const ModuleManager&) = delete;

        // Service must be a versioned C function table owned by the caller.
        bool RegisterService(const std::string& name, std::uint32_t version,
            const void* service, std::string& error);
        bool Load(const std::string& manifestPath, std::string& error);
        void Tick(double deltaTime);
        void UnloadAll();
        std::size_t Count() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
