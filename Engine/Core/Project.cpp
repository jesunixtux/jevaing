#include <Jevaing/Project.h>

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

    bool Project::Load(
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

        return true;
    }

    bool Project::Save(
        const std::string& path,
        const ProjectConfig& config,
        std::string& error
    )
    {
        error.clear();
        std::ofstream file(path);

        if (!file)
        {
            error = "Failed to write project file: " + path;
            return false;
        }

        file
            << "name=" << config.Name << "\n"
            << "startupScene=" << config.StartupScene << "\n"
            << "assetRoot=" << config.AssetRoot << "\n"
            << "sceneRoot=" << config.SceneRoot << "\n";

        return true;
    }

    std::string Project::ResolvePath(
        const ProjectConfig& config,
        const std::string& projectRelativePath
    )
    {
        const std::filesystem::path path(projectRelativePath);

        if (path.is_absolute())
        {
            return path.string();
        }

        return (std::filesystem::path(config.ProjectDirectory) / path).string();
    }

    std::string Project::ResolveAssetPath(
        const ProjectConfig& config,
        const std::string& assetRelativePath
    )
    {
        const std::filesystem::path path(assetRelativePath);

        if (path.is_absolute())
        {
            return path.string();
        }

        return (
            std::filesystem::path(config.ProjectDirectory) /
            std::filesystem::path(config.AssetRoot) /
            path
        ).string();
    }

    std::string Project::ResolveStartupScenePath(const ProjectConfig& config)
    {
        return ResolvePath(config, config.StartupScene);
    }
}
