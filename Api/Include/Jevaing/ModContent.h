#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace Jevaing
{
    enum class ModContentKind { Asset, Map };

    struct ModPolicy
    {
        bool Enabled = false;
        bool AllowAssets = false;
        bool AllowMaps = false;
        bool AllowAssetOverrides = false;
        bool AllowMapOverrides = false;
    };

    // Resolve content from mounted packages without changing base files.
    // A game links JevaingMods and opts in through ModPolicy to use this class.
    class ModContentManager
    {
    public:
        ModContentManager(std::string baseRoot, std::string modsRoot,
            std::string gameName, std::uint32_t gameMajor, ModPolicy policy);
        ~ModContentManager();
        ModContentManager(const ModContentManager&) = delete;
        ModContentManager& operator=(const ModContentManager&) = delete;

        bool Mount(const std::string& manifestPath, std::string& error);
        bool Resolve(ModContentKind kind, const std::string& logicalPath,
            std::string& physicalPath, std::string& error) const;
        void UnmountAll();
        std::size_t Count() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
