#include <Jevaing/Project.h>
#include "AtomicFile.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace Jevaing
{
    namespace
    {
        namespace fs = std::filesystem;

        bool Inside(const fs::path& root, const fs::path& candidate)
        {
            auto parent = root.begin();
            auto child = candidate.begin();
            while (parent != root.end() && child != candidate.end() && *parent == *child)
            {
                ++parent;
                ++child;
            }
            return parent == root.end();
        }

        bool ValidName(const std::string& name)
        {
            return !name.empty() && name.size() <= 64 &&
                std::isalpha(static_cast<unsigned char>(name.front())) &&
                std::all_of(name.begin(), name.end(), [](unsigned char c)
                { return std::isalnum(c) || c == '_' || c == '-'; });
        }
        std::string Trim(const std::string& value)
        {
            const auto begin = std::find_if_not(
                value.begin(),
                value.end(),
                [](unsigned char character)
                {
                    return std::isspace(character) != 0;
                }
            );

            const auto end = std::find_if_not(
                value.rbegin(),
                value.rend(),
                [](unsigned char character)
                {
                    return std::isspace(character) != 0;
                }
            ).base();

            if (begin >= end)
            {
                return {};
            }

            return std::string(begin, end);
        }

        bool ReadKeyValue(
            const std::string& line,
            std::string& key,
            std::string& value
        )
        {
            const std::size_t separator = line.find('=');

            if (separator == std::string::npos)
            {
                return false;
            }

            key = Trim(line.substr(0, separator));
            value = Trim(line.substr(separator + 1));
            return !key.empty();
        }
    }

    bool Project::ResolvePathChecked(const ProjectConfig& config,
        const std::string& relativePath, std::string& resolved, std::string& error)
    {
        resolved.clear();
        if (config.ProjectDirectory.empty() || relativePath.empty())
        {
            error = "Project directory and relative path are required.";
            return false;
        }
        const fs::path path(relativePath);
        if (path.is_absolute() || path.has_root_name() || path.has_root_directory())
        {
            error = "Project path must be relative: " + relativePath;
            return false;
        }
        for (const auto& component : path)
            if (component == "..")
            {
                error = "Parent traversal is not allowed in project paths: " + relativePath;
                return false;
            }
        std::error_code ec;
        const fs::path root = fs::weakly_canonical(config.ProjectDirectory, ec);
        if (ec) { error = "Invalid project directory: " + ec.message(); return false; }
        const fs::path target = fs::weakly_canonical(root / path, ec);
        if (ec || !Inside(root, target) || target == root)
        {
            error = "Project path escapes its directory: " + relativePath;
            return false;
        }
        resolved = target.string();
        return true;
    }

    bool Project::Validate(const ProjectConfig& config, std::string& error)
    {
        if (!ValidName(config.Name))
        {
            error = "Project name must start with a letter and contain only letters, digits, - or _.";
            return false;
        }
        std::string resolved;
        for (const std::string* path : {&config.StartupScene, &config.AssetRoot, &config.SceneRoot})
            if (!ResolvePathChecked(config, *path, resolved, error)) return false;
        std::string startup, sceneRoot;
        std::string assetRoot;
        if (!ResolvePathChecked(config, config.StartupScene, startup, error) ||
            !ResolvePathChecked(config, config.SceneRoot, sceneRoot, error) ||
            !ResolvePathChecked(config, config.AssetRoot, assetRoot, error) ||
            !Inside(fs::path(sceneRoot), fs::path(startup)))
        {
            if (error.empty()) error = "Startup scene must be inside the scene directory.";
            return false;
        }
        if (Inside(fs::path(sceneRoot), fs::path(assetRoot)) ||
            Inside(fs::path(assetRoot), fs::path(sceneRoot)))
        {
            error = "Asset and scene directories must be separate.";
            return false;
        }
        return true;
    }

    bool Project::Load(
        const std::string& path,
        ProjectConfig& config,
        std::string& error
    )
    {
        if (LoadSingle(path, config, error)) return true;

        const std::string primaryError = error;
        const fs::path backup(path + ".bak");
        std::error_code ec;
        if (fs::is_symlink(backup, ec) || ec || !fs::is_regular_file(backup, ec))
        {
            error = primaryError;
            return false;
        }

        if (LoadSingle(backup.string(), config, error)) return true;
        error = primaryError + " Backup recovery failed: " + error;
        return false;
    }

    bool Project::LoadSingle(
        const std::string& path,
        ProjectConfig& config,
        std::string& error
    )
    {
        config = {};
        error.clear();

        std::ifstream file(path);

        if (!file)
        {
            error = "Failed to open project file: " + path;
            return false;
        }

        config.ProjectDirectory = std::filesystem::absolute(
            std::filesystem::path(path)
        ).parent_path().string();

        std::string line;
        int lineNumber = 0;

        while (std::getline(file, line))
        {
            ++lineNumber;
            line = Trim(line);

            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            std::string key;
            std::string value;

            if (!ReadKeyValue(line, key, value))
            {
                error =
                    "Invalid project file line " +
                    std::to_string(lineNumber) +
                    " in " +
                    path;
                return false;
            }

            if (key == "name")
            {
                config.Name = value;
            }
            else if (key == "startupScene")
            {
                config.StartupScene = value;
            }
            else if (key == "assetRoot")
            {
                config.AssetRoot = value;
            }
            else if (key == "sceneRoot")
            {
                config.SceneRoot = value;
            }
        }

        if (config.Name.empty())
        {
            error = "Project file is missing required key: name";
            return false;
        }

        if (config.StartupScene.empty())
        {
            error = "Project file is missing required key: startupScene";
            return false;
        }

        if (config.AssetRoot.empty())
        {
            config.AssetRoot = "Assets";
        }

        if (config.SceneRoot.empty())
        {
            config.SceneRoot = "Scenes";
        }

        return Validate(config, error);
    }

    bool Project::Save(
        const std::string& path,
        const ProjectConfig& config,
        std::string& error
    )
    {
        error.clear();
        if (!Validate(config, error)) return false;
        return Internal::WriteAtomicFile(path, [&](std::ostream& file)
        {
            file << "name=" << config.Name << "\n"
                 << "startupScene=" << config.StartupScene << "\n"
                 << "assetRoot=" << config.AssetRoot << "\n"
                 << "sceneRoot=" << config.SceneRoot << "\n";
        }, error);
    }

    std::string Project::ResolvePath(
        const ProjectConfig& config,
        const std::string& projectRelativePath
    )
    {
        std::string resolved, error;
        return ResolvePathChecked(config, projectRelativePath, resolved, error) ? resolved : std::string{};
    }

    std::string Project::ResolveAssetPath(
        const ProjectConfig& config,
        const std::string& assetRelativePath
    )
    {
        return ResolvePath(config, (fs::path(config.AssetRoot) / assetRelativePath).string());
    }

    std::string Project::ResolveStartupScenePath(const ProjectConfig& config)
    {
        return ResolvePath(config, config.StartupScene);
    }
}
