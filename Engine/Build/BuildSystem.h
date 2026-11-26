#pragma once

#include <string>
#include <vector>
#include <Jevaing/Project.h>

namespace Jevaing::Internal
{
    enum class BuildTarget
    {
        WindowsDesktopX64,
        XboxDevModeUwpX64,
        XboxOneGdkX64,
        XboxSeriesGdkX64
    };

    enum class BuildConfiguration
    {
        Debug,
        Release
    };

    struct BuildTargetInfo
    {
        BuildTarget Target = BuildTarget::WindowsDesktopX64;
        std::string DisplayName;
        bool Available = false;
        bool Experimental = false;
        std::string Reason;
    };

    struct BuildSettings
    {
        BuildTarget Target = BuildTarget::WindowsDesktopX64;
        BuildConfiguration Configuration = BuildConfiguration::Debug;
        std::string OutputDirectory = "Builds";
        std::string StartupScene;
        std::vector<std::string> ScenesInBuild;
        bool TrustProjectBuildScripts = false;
        bool SmokeTestExport = false;
    };

    struct BuildResult
    {
        bool Success = false;
        std::string Message;
        std::string OutputPath;
    };

    const char* BuildTargetToString(BuildTarget target);
    std::vector<BuildTargetInfo> GetBuildTargetInfo();

    bool CreateProjectFromTemplate(
        const std::string& projectName,
        const std::string& parentDirectory,
        std::string& projectFile,
        std::string& error
    );

    BuildResult BuildWindowsDesktop(
        const ProjectConfig& project,
        const BuildSettings& settings,
        bool runAfterBuild
    );

    bool EditorSceneRoundTripTest(std::string& error);
    bool PlayModeRestoreTest(std::string& error);
}
