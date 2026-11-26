#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <Jevaing/JevaingModule.h>
#include <Jevaing/ModuleManager.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Jevaing
{
    namespace
    {
        namespace fs = std::filesystem;

        struct Dependency { std::string Name; std::uint32_t Major = 0; };
        struct Manifest
        {
            std::string Name;
            std::uint32_t Major = 0;
            std::string Version;
            fs::path Library;
            std::vector<Dependency> Dependencies;
        };

        std::string Trim(const std::string& value)
        {
            const auto first = value.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) return {};
            const auto last = value.find_last_not_of(" \t\r\n");
            return value.substr(first, last - first + 1);
        }

        bool ParseNumber(const std::string& value, std::uint32_t& number)
        {
            if (value.empty()) return false;
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
            return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size();
        }

        bool ValidName(const std::string& name)
        {
            if (name.empty() || name.size() > 64 ||
                !((name[0] >= 'A' && name[0] <= 'Z') ||
                  (name[0] >= 'a' && name[0] <= 'z'))) return false;
            return std::all_of(name.begin(), name.end(), [](unsigned char c)
            {
                return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') || c == '_' || c == '-';
            });
        }

        bool Within(const fs::path& root, const fs::path& candidate)
        {
            auto a = root.begin(), b = candidate.begin();
            while (a != root.end() && b != candidate.end() && *a == *b) { ++a; ++b; }
            return a == root.end();
        }

        bool ParseManifest(const fs::path& path, Manifest& manifest, std::string& error)
        {
            std::ifstream file(path);
            if (!file) { error = "Cannot open module manifest: " + path.string(); return false; }
            bool sawName = false, sawVersion = false, sawAbi = false, sawLibrary = false;
            std::string line;
            unsigned lineNumber = 0;
            while (std::getline(file, line))
            {
                ++lineNumber;
                line = Trim(line);
                if (line.empty() || line.front() == '#') continue;
                const auto equals = line.find('=');
                if (equals == std::string::npos)
                { error = "Invalid module manifest line " + std::to_string(lineNumber); return false; }
                const auto key = Trim(line.substr(0, equals));
                const auto value = Trim(line.substr(equals + 1));
                if (key == "name" && !sawName) { manifest.Name = value; sawName = true; }
                else if (key == "version" && !sawVersion)
                {
                    manifest.Version = value;
                    const auto dot = value.find('.');
                    const auto second = dot == std::string::npos ? dot : value.find('.', dot + 1);
                    std::uint32_t minor = 0, patch = 0;
                    if (dot == std::string::npos || second == std::string::npos ||
                        !ParseNumber(value.substr(0, dot), manifest.Major) ||
                        !ParseNumber(value.substr(dot + 1, second - dot - 1), minor) ||
                        !ParseNumber(value.substr(second + 1), patch) || manifest.Major == 0)
                    { error = "Invalid module version: " + value; return false; }
                    sawVersion = true;
                }
                else if (key == "abi" && !sawAbi)
                {
                    std::uint32_t abi = 0;
                    if (!ParseNumber(value, abi) || abi != JEVAING_MODULE_ABI_VERSION)
                    { error = "Incompatible module ABI: " + value; return false; }
                    sawAbi = true;
                }
                else if (key == "library" && !sawLibrary)
                { manifest.Library = fs::u8path(value); sawLibrary = true; }
                else if (key == "dependency")
                {
                    const auto at = value.find('@');
                    Dependency dep;
                    if (at == std::string::npos || !ValidName(value.substr(0, at)) ||
                        !ParseNumber(value.substr(at + 1), dep.Major) || dep.Major == 0)
                    { error = "Invalid module dependency: " + value; return false; }
                    dep.Name = value.substr(0, at);
                    manifest.Dependencies.push_back(std::move(dep));
                }
                else { error = "Unknown or duplicate module key: " + key; return false; }
            }
            if (!file.eof()) { error = "Cannot finish reading module manifest."; return false; }
            if (!sawName || !ValidName(manifest.Name) || !sawVersion || !sawAbi ||
                !sawLibrary || manifest.Library.empty() || manifest.Library.is_absolute() ||
                manifest.Library.has_root_name() || manifest.Library.extension() != ".dll")
            { error = "Module manifest has missing or invalid fields."; return false; }
            for (const auto& part : manifest.Library)
                if (part == "..") { error = "Module library path cannot traverse parents."; return false; }
            return true;
        }
    }

    struct ModuleManager::Impl
    {
        struct Loaded
        {
            std::string Name;
            std::uint32_t Major = 0;
            fs::path ManifestPath;
            HMODULE Library = nullptr;
            jevaing_module_v1 Api{};
        };
        std::vector<Loaded> Modules;
        std::map<std::string, std::size_t> ByName;
        std::map<std::pair<std::string, std::uint32_t>, const void*> Services;
        std::set<fs::path> Loading;
        jevaing_module_host_v1 Host{};

        Impl()
        {
            Host.abi_version = JEVAING_MODULE_ABI_VERSION;
            Host.struct_size = sizeof(Host);
            Host.user_data = this;
            Host.log = [](void*, int32_t level, const char* message)
            {
                if (message) std::clog << "[Jevaing][Module][" << level << "] " << message << '\n';
            };
            Host.get_service = [](void* user, const char* name, std::uint32_t version) -> const void*
            {
                if (!user || !name) return nullptr;
                try {
                    const auto& services = static_cast<Impl*>(user)->Services;
                    const auto found = services.find({name, version});
                    return found == services.end() ? nullptr : found->second;
                } catch (...) { return nullptr; }
            };
        }

        void UnloadFrom(std::size_t first)
        {
            while (Modules.size() > first)
            {
                Loaded module = std::move(Modules.back());
                Modules.pop_back();
                ByName.erase(module.Name);
                if (module.Api.on_unload)
                {
                    try { module.Api.on_unload(module.Api.user_data); }
                    catch (...) { std::clog << "[Jevaing][Module] on_unload threw.\n"; }
                }
                if (module.Library) FreeLibrary(module.Library);
            }
        }

        bool LoadOne(const fs::path& manifestPath, const std::string& expectedName,
            std::uint32_t requiredMajor, std::string& error)
        {
            std::error_code ec;
            const fs::path path = fs::weakly_canonical(manifestPath, ec);
            if (ec || path.extension() != ".jevmod")
            { error = "Invalid module manifest path."; return false; }
            if (!Loading.insert(path).second)
            { error = "Module dependency cycle at " + path.string(); return false; }
            struct LoadingGuard
            {
                std::set<fs::path>& Set;
                fs::path Path;
                ~LoadingGuard() { Set.erase(Path); }
            } guard{Loading, path};

            Manifest manifest;
            if (!ParseManifest(path, manifest, error)) return false;
            if (!expectedName.empty() && manifest.Name != expectedName)
            { error = "Module dependency name does not match its manifest."; return false; }
            if (requiredMajor && manifest.Major != requiredMajor)
            { error = "Module major version is incompatible: " + manifest.Name; return false; }
            const auto existing = ByName.find(manifest.Name);
            if (existing != ByName.end())
            {
                if (Modules[existing->second].Major != manifest.Major ||
                    Modules[existing->second].ManifestPath != path)
                { error = "Already loaded module has an incompatible version or path."; return false; }
                return true;
            }
            for (const Dependency& dep : manifest.Dependencies)
            {
                if (!LoadOne(path.parent_path() / (dep.Name + ".jevmod"),
                    dep.Name, dep.Major, error)) return false;
            }
            const fs::path library = fs::weakly_canonical(path.parent_path() / manifest.Library, ec);
            if (ec || !Within(path.parent_path(), library) || !fs::is_regular_file(library))
            { error = "Module library is missing or escapes its manifest directory."; return false; }
            HMODULE handle = LoadLibraryExW(library.c_str(), nullptr,
                LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
            if (!handle)
            { error = "Cannot load module DLL: Windows error " + std::to_string(GetLastError()); return false; }
            const auto query = reinterpret_cast<jevaing_module_query_fn>(
                GetProcAddress(handle, "jevaing_module_query"));
            jevaing_module_v1 api{};
            bool queried = false;
            try { queried = query && query(JEVAING_MODULE_ABI_VERSION, &api) == 0; }
            catch (...) { queried = false; }
            if (!queried ||
                api.abi_version != JEVAING_MODULE_ABI_VERSION ||
                api.struct_size != sizeof(api) || !api.on_load || !api.on_unload)
            { FreeLibrary(handle); error = "Module interface is incompatible."; return false; }
            int32_t loaded = -1;
            try { loaded = api.on_load(api.user_data, &Host); }
            catch (...) { error = "Module on_load threw across its C interface."; }
            if (loaded != 0)
            {
                try { api.on_unload(api.user_data); } catch (...) {}
                FreeLibrary(handle);
                if (error.empty()) error = "Module on_load failed.";
                return false;
            }
            const auto previousCount = Modules.size();
            try {
                Modules.push_back({manifest.Name, manifest.Major, path, handle, api});
                ByName.emplace(manifest.Name, previousCount);
            } catch (...) {
                if (Modules.size() > previousCount) UnloadFrom(previousCount);
                else {
                    try { api.on_unload(api.user_data); } catch (...) {}
                    FreeLibrary(handle);
                }
                throw;
            }
            return true;
        }
    };

    ModuleManager::ModuleManager() : m_impl(std::make_unique<Impl>()) {}
    ModuleManager::~ModuleManager() { UnloadAll(); }

    bool ModuleManager::RegisterService(const std::string& name, std::uint32_t version,
        const void* service, std::string& error)
    {
        error.clear();
        if (!ValidName(name) || !version || !service)
        { error = "Invalid service name, version or pointer."; return false; }
        if (!m_impl->Services.emplace(std::make_pair(name, version), service).second)
        { error = "Service version is already registered."; return false; }
        return true;
    }

    bool ModuleManager::Load(const std::string& manifestPath, std::string& error)
    {
        error.clear();
        const std::size_t originalCount = m_impl->Modules.size();
        try {
            if (m_impl->LoadOne(fs::u8path(manifestPath), {}, 0, error)) return true;
        } catch (const std::exception& exception) { error = exception.what(); }
          catch (...) { error = "Unknown module load failure."; }
        m_impl->UnloadFrom(originalCount);
        return false;
    }

    void ModuleManager::Tick(double deltaTime)
    {
        if (!std::isfinite(deltaTime) || deltaTime < 0) return;
        for (const auto& module : m_impl->Modules)
        {
            if (module.Api.on_tick)
            {
                try { module.Api.on_tick(module.Api.user_data, deltaTime); }
                catch (...) { std::clog << "[Jevaing][Module] on_tick threw.\n"; }
            }
        }
    }

    void ModuleManager::UnloadAll() { m_impl->UnloadFrom(0); }
    std::size_t ModuleManager::Count() const { return m_impl->Modules.size(); }
}
