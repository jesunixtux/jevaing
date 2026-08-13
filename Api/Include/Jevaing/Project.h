#pragma once

#include <string>

namespace Jevaing
{
    struct ProjectConfig
    {
        std::string Name;
        std::string StartupScene;
        std::string AssetRoot = "Assets";
        std::string SceneRoot = "Scenes";
        std::string ProjectDirectory;
    };

    class Project
    {
    public:
        static bool Load(
            const std::string& path,
            ProjectConfig& config,
            std::string& error
        );

        static bool Save(
            const std::string& path,
            const ProjectConfig& config,
            std::string& error
        );

        static std::string ResolvePath(
            const ProjectConfig& config,
            const std::string& projectRelativePath
        );

        static std::string ResolveAssetPath(
            const ProjectConfig& config,
            const std::string& assetRelativePath
        );

        static std::string ResolveStartupScenePath(const ProjectConfig& config);
    };
}
