#include "BuildSystem.h"

#include <Jevaing/Jevaing.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace Jevaing::Internal
{
    namespace
    {
        namespace fs = std::filesystem;

        bool IsValidProjectName(const std::string& name)
        {
            if (name.empty() || name.size() > 64 || !std::isalpha(static_cast<unsigned char>(name[0])))
                return false;

            return std::all_of(name.begin(), name.end(), [](unsigned char c)
            {
                return std::isalnum(c) || c == '_' || c == '-';
            });
        }

        void ReplaceAll(std::string& value, const std::string& from, const std::string& to)
        {
            std::size_t position = 0;
            while ((position = value.find(from, position)) != std::string::npos)
            {
                value.replace(position, from.length(), to);
                position += to.length();
            }
        }

        bool WriteFile(const fs::path& path, const std::string& content, std::string& error)
        {
            std::ofstream out(path, std::ios::binary);
            if (!out || !(out << content))
            {
                error = "Cannot write: " + path.string();
                return false;
            }
            return true;
        }

        const char* ConfigurationName(BuildConfiguration configuration)
        {
            return configuration == BuildConfiguration::Release ? "Release" : "Debug";
        }

        bool CopyDirectory(const fs::path& source, const fs::path& destination, std::string& error)
        {
            std::error_code ec;
            fs::create_directories(destination, ec);
            if (ec)
            {
                error = "Cannot create destination: " + ec.message();
                return false;
            }
            if (!fs::exists(source))
                return true;

            fs::copy(source, destination,
                fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
            if (ec)
            {
                error = "Cannot copy assets: " + ec.message();
                return false;
            }
            return true;
        }

        // Only the project's own source is compiled. The engine source path is
        // passed at build time, never written into the generated CMake project.
        const char* ProjectCmake = R"JEVAING(cmake_minimum_required(VERSION 3.20)
project(@NAME@ LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(JEVAING_ENGINE_ROOT "" CACHE PATH "Path to Jevaing engine source")
if(NOT EXISTS "@DOLLAR@{JEVAING_ENGINE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "Set JEVAING_ENGINE_ROOT to the Jevaing source directory.")
endif()
set(JEVAING_BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
set(JEVAING_BUILD_EDITOR OFF CACHE BOOL "" FORCE)
set(JEVAING_BUILD_XBOX_SMOKE_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory("@DOLLAR@{JEVAING_ENGINE_ROOT}" JevaingBuild)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "Source/*.cpp"
    "Source/*.c"
)
add_executable(@NAME@ @DOLLAR@{GAME_SOURCES})
target_include_directories(@NAME@ PRIVATE Source)
target_link_libraries(@NAME@ PRIVATE JevaingEngine)
set_target_properties(@NAME@ PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "@DOLLAR@{CMAKE_CURRENT_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "@DOLLAR@{CMAKE_CURRENT_BINARY_DIR}/bin/Release"
)
add_custom_command(TARGET @NAME@ POST_BUILD
    COMMAND "@DOLLAR@{CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:@NAME@>/Assets"
    COMMAND "@DOLLAR@{CMAKE_COMMAND}" -E copy_directory "@DOLLAR@{CMAKE_CURRENT_SOURCE_DIR}/Assets" "$<TARGET_FILE_DIR:@NAME@>/Assets"
    COMMAND "@DOLLAR@{CMAKE_COMMAND}" -E copy_directory "@DOLLAR@{CMAKE_CURRENT_SOURCE_DIR}/Scenes" "$<TARGET_FILE_DIR:@NAME@>/Scenes"
    COMMAND "@DOLLAR@{CMAKE_COMMAND}" -E copy_if_different "@DOLLAR@{CMAKE_CURRENT_SOURCE_DIR}/jevaing.project" "$<TARGET_FILE_DIR:@NAME@>/jevaing.project"
    VERBATIM
)
)JEVAING";

        const char* GameHeader = R"JEVAING(#pragma once
#ifdef __cplusplus
extern "C" {
#endif
float JevaingDemoSpeed(void);
#ifdef __cplusplus
}
#endif
)JEVAING";

        const char* GameC = R"JEVAING(#include "GameCode.h"
float JevaingDemoSpeed(void) { return 2.0f; }
)JEVAING";

        const char* GameCpp = R"JEVAING(#include <Jevaing/Jevaing.h>
#include "GameCode.h"

#include <filesystem>
#include <iostream>
#include <string>

class StarterGame final : public Jevaing::Game
{
public:
    void OnStart() override
    {
        std::string error;
        if (!Jevaing::Project::Load("jevaing.project", m_project, error))
        {
            std::cerr << error << '\n';
            m_failed = true;
            return;
        }

        const std::string scenePath = Jevaing::Project::ResolveStartupScenePath(m_project);
        const std::string assetRoot = Jevaing::Project::ResolvePath(m_project, m_project.AssetRoot);
        if (!m_scene.Load(scenePath, assetRoot, error))
        {
            std::cerr << error << '\n';
            m_failed = true;
        }
    }

    void OnUpdate(double deltaTime) override
    {
        if (m_failed) return;
        const float delta = static_cast<float>(deltaTime) * JevaingDemoSpeed();
        if (Jevaing::Input::IsKeyDown(Jevaing::Key::A)) m_cube.Position.X -= delta;
        if (Jevaing::Input::IsKeyDown(Jevaing::Key::D)) m_cube.Position.X += delta;
        if (Jevaing::Input::IsKeyDown(Jevaing::Key::W)) m_cube.Position.Y += delta;
        if (Jevaing::Input::IsKeyDown(Jevaing::Key::S)) m_cube.Position.Y -= delta;
        m_scene.Update(deltaTime);
    }

    void OnRender(Jevaing::Graphics2D& graphics) override
    {
        graphics.Clear({0.04f, 0.05f, 0.08f, 1.0f});
        if (!m_failed) m_scene.Render(graphics);
    }

    void OnRender(Jevaing::Graphics3D& graphics) override
    {
        if (m_failed) return;
        m_scene.Render(graphics);
        graphics.DrawCube(m_cube, {0.25f, 0.65f, 0.95f, 1.0f});
    }

private:
    Jevaing::ProjectConfig m_project;
    Jevaing::Scene m_scene;
    Jevaing::Transform m_cube;
    bool m_failed = false;
};

int main(int argc, char** argv)
{
    // Resolve content relative to the executable, not the user's shell.
    if (argc > 0 && argv && argv[0])
    {
        std::error_code error;
        const auto executable = std::filesystem::absolute(argv[0], error);
        if (!error && executable.has_parent_path())
            std::filesystem::current_path(executable.parent_path(), error);
    }

    StarterGame game;
    Jevaing::GameConfig config;
    config.Title = "Jevaing Starter";
    config.Width = 1280;
    config.Height = 720;
    return Jevaing::Run(game, config, argc, argv);
}
)JEVAING";

        bool SaveAndRestoreSceneTest(bool playMode, std::string& error)
        {
            error.clear();
            const fs::path file = fs::temp_directory_path() /
                (playMode ? "jevaing-play-restore-test.scene" : "jevaing-roundtrip-test.scene");

            Scene editable("test");
            const EntityId object = editable.CreateEntity("Box");
            editable.FindEntity(object)->Transform.LocalTransform.Position = {1.0f, 2.0f, 3.0f};
            const EntityId child = editable.CreateEntity("Child");
            if (!editable.SetParent(child, object, &error))
                return false;
            if (!editable.Save(file.string(), error))
                return false;

            Scene loaded;
            const bool read = loaded.Load(file.string(), "", error);
            std::error_code ignored;
            fs::remove(file, ignored);
            if (!read)
                return false;

            auto* restored = loaded.FindEntityByName("Box");
            auto* restoredChild = loaded.FindEntityByName("Child");
            if (!restored || !restoredChild || restoredChild->Parent != restored->Id)
            {
                error = "Hierarchy was lost when restoring a scene.";
                return false;
            }
            if (std::fabs(restored->Transform.LocalTransform.Position.X - 1.0f) > 0.001f)
            {
                error = "Scene transform roundtrip failed.";
                return false;
            }

            if (playMode)
            {
                restored->Transform.LocalTransform.Position.X = 99.0f;
                const auto* original = editable.FindEntityByName("Box");
                if (!original || std::fabs(original->Transform.LocalTransform.Position.X - 1.0f) > 0.001f)
                {
                    error = "Play Mode mutated the original edit scene.";
                    return false;
                }
            }
            return true;
        }
    }

    const char* BuildTargetToString(BuildTarget target)
    {
        switch (target)
        {
            case BuildTarget::WindowsDesktopX64: return "Windows Desktop x64";
            case BuildTarget::XboxDevModeUwpX64: return "Xbox Dev Mode / UWP x64";
            case BuildTarget::XboxOneGdkX64: return "Xbox One / GDK x64";
            case BuildTarget::XboxSeriesGdkX64: return "Xbox Series X|S / GDK x64";
        }
        return "Unknown";
    }

    std::vector<BuildTargetInfo> GetBuildTargetInfo()
    {
#ifdef _WIN32
        constexpr bool windows = true;
#else
        constexpr bool windows = false;
#endif
        return {
            {BuildTarget::WindowsDesktopX64, "Windows Desktop x64", windows, false,
                windows ? "Requires CMake and the MSVC toolchain." : "Windows host required."},
            {BuildTarget::XboxDevModeUwpX64, "Xbox Dev Mode / UWP x64", false, true,
                "Experimental smoke test only; packaging and hardware launch not verified."},
            {BuildTarget::XboxOneGdkX64, "Xbox One / GDK x64", false, true,
                "Requires an authorized GDK; commercial build not implemented."},
            {BuildTarget::XboxSeriesGdkX64, "Xbox Series X|S / GDK x64", false, true,
                "Requires an authorized GDK; commercial build not implemented."}
        };
    }

    bool CreateProjectFromTemplate(
        const std::string& name, const std::string& parent,
        std::string& projectFile, std::string& error)
    {
        error.clear();
        projectFile.clear();
        if (!IsValidProjectName(name))
        {
            error = "Project name must start with a letter and contain only letters, digits, - or _.";
            return false;
        }

        const fs::path directory = fs::path(parent) / name;
        if (fs::exists(directory))
        {
            error = "Project directory already exists: " + directory.string();
            return false;
        }

        std::error_code ec;
        fs::create_directories(directory / "Assets" / "Models", ec);
        if (!ec) fs::create_directories(directory / "Assets" / "Textures", ec);
        if (!ec) fs::create_directories(directory / "Scenes", ec);
        if (!ec) fs::create_directories(directory / "Source", ec);
        if (ec)
        {
            error = "Cannot create project: " + ec.message();
            return false;
        }

        ProjectConfig project;
        project.Name = name;
        project.StartupScene = "Scenes/main.scene";
        project.AssetRoot = "Assets";
        project.SceneRoot = "Scenes";
        project.ProjectDirectory = directory.string();

        Scene scene("main");
        const EntityId camera = scene.CreateEntity("Camera");
        scene.FindEntity(camera)->Camera = CameraComponent{};
        scene.FindEntity(camera)->Camera->Primary = true;
        const bool saved =
            Project::Save((directory / "jevaing.project").string(), project, error) &&
            scene.Save((directory / "Scenes" / "main.scene").string(), error);
        if (!saved)
            return false;

        std::string cmake = ProjectCmake;
        ReplaceAll(cmake, "@NAME@", name);
        ReplaceAll(cmake, "@DOLLAR@", "$");
        if (!WriteFile(directory / "CMakeLists.txt", cmake, error) ||
            !WriteFile(directory / "Source" / "Game.cpp", GameCpp, error) ||
            !WriteFile(directory / "Source" / "GameCode.c", GameC, error) ||
            !WriteFile(directory / "Source" / "GameCode.h", GameHeader, error))
            return false;

        projectFile = (directory / "jevaing.project").string();
        return true;
    }

    BuildResult BuildWindowsDesktop(
        const ProjectConfig& project, const BuildSettings& settings, bool runAfterBuild)
    {
        BuildResult result;
#ifndef _WIN32
        (void)project;
        (void)settings;
        (void)runAfterBuild;
        result.Message = "Windows Desktop builds require a Windows development host.";
        return result;
#else
        if (settings.Target != BuildTarget::WindowsDesktopX64)
        {
            result.Message = "Unsupported target for Windows Desktop build.";
            return result;
        }
#ifndef JEVAING_SOURCE_ROOT
        result.Message = "JEVAING_SOURCE_ROOT is not configured.";
        return result;
#else
        const fs::path projectDir = fs::absolute(project.ProjectDirectory);
        const fs::path engineRoot = fs::path(JEVAING_SOURCE_ROOT);
        const fs::path cmakeFile = projectDir / "CMakeLists.txt";
        if (!fs::exists(cmakeFile) || !fs::exists(engineRoot / "CMakeLists.txt"))
        {
            result.Message = "Project or engine CMakeLists.txt not found.";
            return result;
        }

        const std::string config = ConfigurationName(settings.Configuration);
        const fs::path buildDir = projectDir / ".jevaing-build";
        const fs::path outputDir = settings.OutputDirectory.empty()
            ? projectDir / "Builds"
            : (fs::path(settings.OutputDirectory).is_absolute()
                ? fs::path(settings.OutputDirectory)
                : projectDir / settings.OutputDirectory);
        const std::string quotedProject = "\"" + projectDir.string() + "\"";
        const std::string quotedBuild = "\"" + buildDir.string() + "\"";
        const std::string quotedEngine = "\"" + engineRoot.string() + "\"";

        const std::string configure =
            "cmake -S " + quotedProject + " -B " + quotedBuild +
            " -DJEVAING_ENGINE_ROOT:PATH=" + quotedEngine;
        if (std::system(configure.c_str()) != 0)
        {
            result.Message = "CMake configure failed. Check Visual Studio, CMake and dependencies.";
            return result;
        }
        const std::string build =
            "cmake --build " + quotedBuild + " --config " + config +
            " --target " + project.Name;
        if (std::system(build.c_str()) != 0)
        {
            result.Message = "CMake compilation failed.";
            return result;
        }

        const fs::path exe = buildDir / "bin" / config / (project.Name + ".exe");
        if (!fs::exists(exe))
        {
            result.Message = "Build succeeded but game executable was not found: " + exe.string();
            return result;
        }

        std::string error;
        std::error_code ec;
        fs::create_directories(outputDir, ec);
        if (ec)
        {
            result.Message = "Cannot create output directory: " + ec.message();
            return result;
        }
        const fs::path destination = outputDir / exe.filename();
        fs::copy_file(exe, destination, fs::copy_options::overwrite_existing, ec);
        if (ec ||
            !CopyDirectory(projectDir / project.AssetRoot, outputDir / project.AssetRoot, error) ||
            !CopyDirectory(projectDir / project.SceneRoot, outputDir / project.SceneRoot, error))
        {
            result.Message = ec ? ec.message() : error;
            return result;
        }
        fs::copy_file(projectDir / "jevaing.project",
            outputDir / "jevaing.project", fs::copy_options::overwrite_existing, ec);
        if (ec)
        {
            result.Message = "Cannot stage project metadata: " + ec.message();
            return result;
        }

        result.Success = true;
        result.OutputPath = destination.string();
        result.Message = "Game build completed. Validate on a clean Windows installation.";
        if (runAfterBuild)
        {
            const std::string command = "\"" + result.OutputPath + "\"";
            if (std::system(command.c_str()) != 0)
                result.Message += " Game process returned a nonzero status.";
        }
        return result;
#endif
#endif
    }

    bool EditorSceneRoundTripTest(std::string& error)
    {
        return SaveAndRestoreSceneTest(false, error);
    }

    bool PlayModeRestoreTest(std::string& error)
    {
        return SaveAndRestoreSceneTest(true, error);
    }
}
