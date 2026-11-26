#include <Jevaing/ModContent.h>

#include <algorithm>
#include <charconv>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <system_error>
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
            std::string Game;
            std::uint32_t Major = 0;
            std::uint32_t GameMajor = 0;
            fs::path ContentRoot;
            std::vector<Dependency> Dependencies;
        };

        std::string Trim(const std::string& value)
        {
            const auto first = value.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) return {};
            const auto last = value.find_last_not_of(" \t\r\n");
            return value.substr(first, last - first + 1);
        }

        bool Number(const std::string& text, std::uint32_t& value)
        {
            if (text.empty()) return false;
            const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
            return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
        }

        bool Name(const std::string& value)
        {
            if (value.empty() || value.size() > 64 ||
                !((value[0] >= 'A' && value[0] <= 'Z') ||
                  (value[0] >= 'a' && value[0] <= 'z'))) return false;
            return std::all_of(value.begin(), value.end(), [](unsigned char c)
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

        bool Relative(const fs::path& value)
        {
            if (value.empty() || value.is_absolute() || value.has_root_name()) return false;
            for (const auto& part : value) if (part == "..") return false;
            return true;
        }

        bool Parse(const fs::path& filePath, Manifest& manifest, std::string& error)
        {
            std::ifstream file(filePath);
            if (!file) { error = "Cannot read mod manifest: " + filePath.string(); return false; }
            std::set<std::string> seen;
            std::string line;
            unsigned lineNumber = 0;
            while (std::getline(file, line))
            {
                ++lineNumber;
                line = Trim(line);
                if (line.empty() || line.front() == '#') continue;
                const auto equals = line.find('=');
                if (equals == std::string::npos)
                { error = "Invalid mod manifest line " + std::to_string(lineNumber); return false; }
                const auto key = Trim(line.substr(0, equals));
                const auto value = Trim(line.substr(equals + 1));
                if (key != "dependency" && !seen.insert(key).second)
                { error = "Duplicate mod manifest key: " + key; return false; }
                if (key == "name") manifest.Name = value;
                else if (key == "game") manifest.Game = value;
                else if (key == "gameMajor")
                {
                    if (!Number(value, manifest.GameMajor) || !manifest.GameMajor)
                    { error = "Invalid gameMajor."; return false; }
                }
                else if (key == "version")
                {
                    const auto first = value.find('.');
                    const auto second = first == std::string::npos ? first : value.find('.', first + 1);
                    std::uint32_t minor = 0, patch = 0;
                    if (first == std::string::npos || second == std::string::npos ||
                        !Number(value.substr(0, first), manifest.Major) ||
                        !Number(value.substr(first + 1, second - first - 1), minor) ||
                        !Number(value.substr(second + 1), patch) || !manifest.Major)
                    { error = "Invalid mod version."; return false; }
                }
                else if (key == "contentRoot") manifest.ContentRoot = fs::u8path(value);
                else if (key == "dependency")
                {
                    const auto at = value.find('@');
                    Dependency dep;
                    if (at == std::string::npos || !Name(value.substr(0, at)) ||
                        !Number(value.substr(at + 1), dep.Major) || !dep.Major)
                    { error = "Invalid mod dependency: " + value; return false; }
                    dep.Name = value.substr(0, at);
                    manifest.Dependencies.push_back(std::move(dep));
                }
                else { error = "Unknown mod manifest key: " + key; return false; }
            }
            if (!file.eof()) { error = "Cannot finish reading mod manifest."; return false; }
            if (!Name(manifest.Name) || !Name(manifest.Game) || !manifest.Major ||
                !manifest.GameMajor || !Relative(manifest.ContentRoot))
            { error = "Mod manifest is missing required fields."; return false; }
            return true;
        }

        bool ExistingFile(const fs::path& root, const fs::path& relative,
            fs::path& result, std::string& error)
        {
            std::error_code ec;
            const fs::path candidate = fs::weakly_canonical(root / relative, ec);
            if (ec || !Within(root, candidate) || candidate == root)
            { error = "Content path escapes its root."; return false; }
            const fs::file_status status = fs::status(candidate, ec);
            if (ec == std::errc::no_such_file_or_directory)
                return false;
            if (ec)
            {
                error = "Cannot inspect content path: " + ec.message();
                return false;
            }
            if (!fs::is_regular_file(status)) return false;
            result = candidate;
            return true;
        }
    }

    struct ModContentManager::Impl
    {
        struct Mounted { std::string Name; std::uint32_t Major; fs::path Root; fs::path ManifestPath; };
        fs::path BaseRoot;
        fs::path ModsRoot;
        std::string Game;
        std::uint32_t GameMajor;
        ModPolicy Policy;
        std::vector<Mounted> Mounts;
        std::map<std::string, std::size_t> ByName;
        std::set<fs::path> Loading;

        Impl(std::string baseRoot, std::string modsRoot, std::string game,
            std::uint32_t gameMajor, ModPolicy policy)
            : BaseRoot(fs::weakly_canonical(fs::u8path(baseRoot))),
              ModsRoot(fs::weakly_canonical(fs::u8path(modsRoot))),
              Game(std::move(game)), GameMajor(gameMajor), Policy(policy) {}

        bool MountOne(const fs::path& manifestPath, const std::string& expectedName,
            std::uint32_t requiredMajor, std::string& error)
        {
            std::error_code ec;
            const fs::path path = fs::weakly_canonical(manifestPath, ec);
            if (ec || path.filename() != "mod.jevmod" || !Within(ModsRoot, path) ||
                path == ModsRoot)
            { error = "Mod manifest must be inside the mods directory."; return false; }
            if (!Loading.insert(path).second)
            { error = "Mod dependency cycle at " + path.string(); return false; }
            struct Guard
            {
                std::set<fs::path>& Set;
                fs::path Path;
                ~Guard() { Set.erase(Path); }
            } guard{Loading, path};

            Manifest manifest;
            if (!Parse(path, manifest, error)) return false;
            if (manifest.Name != path.parent_path().filename().string() ||
                (!expectedName.empty() && manifest.Name != expectedName))
            { error = "Mod name does not match its package directory."; return false; }
            if (manifest.Game != Game || manifest.GameMajor != GameMajor ||
                (requiredMajor && manifest.Major != requiredMajor))
            { error = "Mod is incompatible with this game or dependency version."; return false; }
            const auto existing = ByName.find(manifest.Name);
            if (existing != ByName.end())
            {
                const auto& mounted = Mounts[existing->second];
                if (mounted.Major != manifest.Major || mounted.ManifestPath != path)
                { error = "A different mod version or package is already mounted."; return false; }
                return true;
            }
            for (const auto& dep : manifest.Dependencies)
                if (!MountOne(ModsRoot / dep.Name / "mod.jevmod", dep.Name, dep.Major, error))
                    return false;
            const fs::path content = fs::weakly_canonical(path.parent_path() / manifest.ContentRoot, ec);
            if (ec || !Within(path.parent_path(), content) || content == path.parent_path() ||
                !fs::is_directory(content))
            { error = "Mod content directory is missing or escapes its package."; return false; }
            const auto index = Mounts.size();
            Mounts.push_back({manifest.Name, manifest.Major, content, path});
            try { ByName.emplace(manifest.Name, index); }
            catch (...) { Mounts.pop_back(); throw; }
            return true;
        }
    };

    ModContentManager::ModContentManager(std::string baseRoot, std::string modsRoot,
        std::string gameName, std::uint32_t gameMajor, ModPolicy policy)
        : m_impl(std::make_unique<Impl>(std::move(baseRoot), std::move(modsRoot),
            std::move(gameName), gameMajor, policy)) {}
    ModContentManager::~ModContentManager() = default;

    bool ModContentManager::Mount(const std::string& manifestPath, std::string& error)
    {
        error.clear();
        if (!m_impl->Policy.Enabled)
        { error = "Mods are disabled for this game."; return false; }
        if (!Name(m_impl->Game) || !m_impl->GameMajor)
        { error = "Invalid game identity for mods."; return false; }
        const auto originalCount = m_impl->Mounts.size();
        try {
            if (m_impl->MountOne(fs::u8path(manifestPath), {}, 0, error)) return true;
        } catch (const std::exception& exception) { error = exception.what(); }
          catch (...) { error = "Unknown mod mount failure."; }
        while (m_impl->Mounts.size() > originalCount)
        {
            m_impl->ByName.erase(m_impl->Mounts.back().Name);
            m_impl->Mounts.pop_back();
        }
        return false;
    }

    bool ModContentManager::Resolve(ModContentKind kind, const std::string& logicalPath,
        std::string& physicalPath, std::string& error) const
    {
        physicalPath.clear();
        error.clear();
        const fs::path relative = fs::u8path(logicalPath);
        if (!Relative(relative))
        { error = "Invalid logical content path."; return false; }
        const bool assets = kind == ModContentKind::Asset;
        const fs::path category = assets ? "Assets" : "Maps";
        const bool allowed = m_impl->Policy.Enabled &&
            (assets ? m_impl->Policy.AllowAssets : m_impl->Policy.AllowMaps);
        const bool overrides = assets ? m_impl->Policy.AllowAssetOverrides :
            m_impl->Policy.AllowMapOverrides;
        try {
            const fs::path baseCategory = fs::weakly_canonical(m_impl->BaseRoot / category);
            if (!Within(m_impl->BaseRoot, baseCategory))
            { error = "Base content category escapes its root."; return false; }
            fs::path found;
            if (!allowed || !overrides)
            {
                if (ExistingFile(baseCategory, relative, found, error))
                { physicalPath = found.string(); return true; }
                if (!error.empty()) return false;
            }
            if (allowed)
            {
                for (auto it = m_impl->Mounts.rbegin(); it != m_impl->Mounts.rend(); ++it)
                {
                    const fs::path modCategory = fs::weakly_canonical(it->Root / category);
                    if (!Within(it->Root, modCategory))
                    { error = "Mod content category escapes its root."; return false; }
                    if (ExistingFile(modCategory, relative, found, error))
                    { physicalPath = found.string(); return true; }
                    if (!error.empty()) return false;
                }
            }
            if (allowed && overrides && ExistingFile(baseCategory, relative, found, error))
            { physicalPath = found.string(); return true; }
            if (error.empty()) error = "Content was not found.";
            return false;
        } catch (const std::exception& exception) { error = exception.what(); return false; }
          catch (...) { error = "Unknown content resolution failure."; return false; }
    }

    void ModContentManager::UnmountAll()
    {
        m_impl->Mounts.clear();
        m_impl->ByName.clear();
    }
    std::size_t ModContentManager::Count() const { return m_impl->Mounts.size(); }
}
