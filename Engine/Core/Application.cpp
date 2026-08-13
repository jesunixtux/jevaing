#include <chrono>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <Jevaing/Game.h>
#include <Jevaing/Input.h>
#include <Jevaing/Jevaing.h>

#include "Application.h"
#include "CommandLine.h"
#include "InputState.h"
#include "Logger.h"
#include "Timer.h"
#include "Window.h"
#include "../Renderer/Renderer.h"

namespace Jevaing::Internal
{
    namespace
    {
        bool ReportTest(bool condition, const std::string& name)
        {
            if (condition)
            {
                Logger::Info("[PASS] " + name);
                return true;
            }

            Logger::Error("[FAIL] " + name);
            return false;
        }

        std::string ResolveTestAssetPath(const std::string& relativePath)
        {
            namespace fs = std::filesystem;

            std::error_code error;
            const fs::path directPath(relativePath);

            if (fs::exists(directPath, error))
            {
                return directPath.string();
            }

#ifdef JEVAING_SOURCE_ROOT
            error.clear();
            const fs::path sourcePath = fs::path(JEVAING_SOURCE_ROOT) / directPath;

            if (fs::exists(sourcePath, error))
            {
                return sourcePath.string();
            }
#endif

            return directPath.string();
        }

        bool PrepareModelAsset(
            const std::string& relativePath,
            std::shared_ptr<const Model>& outputModel,
            std::string& error
        )
        {
            const std::string resolvedPath = ResolveTestAssetPath(relativePath);

            if (!std::filesystem::exists(std::filesystem::path(resolvedPath)))
            {
                error = "Model file does not exist: " + relativePath;
                return false;
            }

            outputModel = Assets::LoadModel(resolvedPath, &error);

            if (!outputModel)
            {
                return false;
            }

            Logger::Info(
                "Loaded external 3D test model: " +
                relativePath +
                " (" +
                std::to_string(outputModel->TriangleCount()) +
                " triangles)"
            );
            return true;
        }

        std::size_t CountTextureReferences(const Model& model)
        {
            std::size_t count = 0;

            for (const Material& material : model.Materials)
            {
                if (!material.BaseColorTexturePath.empty() || material.BaseColorTexture)
                {
                    ++count;
                }
            }

            return count;
        }

        void PrintBounds(const Bounds3D& bounds)
        {
            if (!bounds.Valid)
            {
                std::cout << "Bounds: unknown" << std::endl;
                return;
            }

            std::cout
                << "Bounds: min("
                << bounds.Min.X << ", "
                << bounds.Min.Y << ", "
                << bounds.Min.Z << ") max("
                << bounds.Max.X << ", "
                << bounds.Max.Y << ", "
                << bounds.Max.Z << ")"
                << std::endl;
        }

        int RunAssetInfo(const std::string& path)
        {
            std::shared_ptr<const Model> model;
            std::string error;

            if (!PrepareModelAsset(path, model, error))
            {
                Logger::Error(error);
                return 2;
            }

            std::cout << "Path: " << model->SourcePath << std::endl;
            std::cout << "Format: " << model->Format << std::endl;
            std::cout << "Meshes: " << model->Meshes.size() << std::endl;
            std::cout << "Vertices: " << model->VertexCount() << std::endl;
            std::cout << "Indices: " << model->IndexCount() << std::endl;
            std::cout << "Triangles: " << model->TriangleCount() << std::endl;
            std::cout << "Materials: " << model->Materials.size() << std::endl;
            std::cout << "Textures: " << CountTextureReferences(*model) << std::endl;
            std::cout << "Normals: " << (model->HasNormals() ? "yes" : "no") << std::endl;
            std::cout << "UV0: " << (model->HasUV0() ? "yes" : "no") << std::endl;
            PrintBounds(model->Bounds);
            return 0;
        }

        int RunAssetCacheTest()
        {
            constexpr const char* TuxPath = "geometry/3D/.hide/easter/tux.glb";

            Assets::ClearCache();

            std::string error;
            auto first = Assets::LoadModel(ResolveTestAssetPath(TuxPath), &error);
            auto second = Assets::LoadModel(ResolveTestAssetPath(TuxPath), &error);
            auto third = Assets::LoadModel(ResolveTestAssetPath(TuxPath), &error);

            const std::size_t importCount =
                Assets::GetModelImportCountForPath(ResolveTestAssetPath(TuxPath));

            const bool success =
                first &&
                second &&
                third &&
                first.get() == second.get() &&
                second.get() == third.get() &&
                importCount == 1;

            if (success)
            {
                Logger::Info("[PASS] AssetManager cache reused the model import once.");
                return 0;
            }

            Logger::Error(
                "[FAIL] AssetManager cache test failed. Import count: " +
                std::to_string(importCount) +
                " Error: " +
                error
            );
            return 2;
        }

        int RunAssetErrorTest()
        {
            bool success = true;
            std::string error;

            auto missing = Assets::LoadModel("geometry/3D/does-not-exist.glb", &error);
            success &= ReportTest(!missing && !error.empty(), "Missing model path fails cleanly");

            auto unsupported = Assets::LoadModel("README.md", &error);
            success &= ReportTest(!unsupported && !error.empty(), "Unsupported extension fails cleanly");

            const std::filesystem::path invalidPath =
                std::filesystem::temp_directory_path() / "jevaing-invalid-model.glb";

            {
                std::ofstream invalidFile(invalidPath, std::ios::binary);
                invalidFile << "not a real glb";
            }

            auto invalid = Assets::LoadModel(invalidPath.string(), &error);
            success &= ReportTest(!invalid && !error.empty(), "Invalid model data fails cleanly");

            std::error_code removeError;
            std::filesystem::remove(invalidPath, removeError);

            if (success)
            {
                Logger::Info("[PASS] Asset error paths completed without crashes.");
                return 0;
            }

            Logger::Error("[FAIL] Asset error paths did not behave as expected.");
            return 2;
        }

        int RunSelfTests()
        {
            Logger::Info("Running Jevaing self-tests...");

            bool success = true;

            success &= ReportTest(
                Jevaing::GetVersion() != nullptr && std::string(Jevaing::GetVersion()).size() > 0,
                "Version string is available"
            );

            success &= ReportTest(
                Jevaing::GetCodename() != nullptr && std::string(Jevaing::GetCodename()).size() > 0,
                "Codename is available"
            );

            Timer timer;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            const double deltaTime = timer.Tick();

            success &= ReportTest(
                deltaTime > 0.0,
                "Timer produces a positive delta time"
            );

            const Vec3 cross = Cross(
                { 1.0f, 0.0f, 0.0f },
                { 0.0f, 1.0f, 0.0f }
            );
            success &= ReportTest(
                cross.Z == 1.0f,
                "Vec3 cross product follows the 3D basis"
            );

            Transform transform;
            transform.Position = { 1.0f, 2.0f, 3.0f };
            const Mat4 model = transform.ToMatrix();
            success &= ReportTest(
                model.M[3][0] == 1.0f &&
                model.M[3][1] == 2.0f &&
                model.M[3][2] == 3.0f,
                "Transform produces a 3D model matrix"
            );

            PerspectiveCamera camera;
            camera.AspectRatio = 16.0f / 9.0f;
            const Mat4 projection = camera.GetProjectionMatrix();
            success &= ReportTest(
                projection.M[0][0] > 0.0f &&
                projection.M[1][1] > 0.0f &&
                projection.M[2][3] == 1.0f,
                "PerspectiveCamera produces a perspective projection"
            );

            RendererBackend parsedBackend = RendererBackend::None;
            success &= ReportTest(
                RendererBackendFromString("directx", parsedBackend) &&
                parsedBackend == RendererBackend::DirectX,
                "Renderer parser recognizes DirectX"
            );

            success &= ReportTest(
                RendererBackendFromString("null", parsedBackend) &&
                parsedBackend == RendererBackend::None,
                "Renderer parser recognizes Null Renderer"
            );

            success &= ReportTest(
                Renderer::IsBackendAvailable(RendererBackend::None),
                "Null Renderer is available"
            );

            const RendererBackend defaultBackend = Renderer::GetDefaultBackend();
            success &= ReportTest(
                Renderer::IsBackendAvailable(defaultBackend),
                std::string("Default renderer is available: ") +
                RendererBackendToString(defaultBackend)
            );

            InputState::Reset();
            success &= ReportTest(
                !Jevaing::Input::IsKeyDown(Jevaing::Key::W),
                "Input starts in a released state"
            );

            Jevaing::GameConfig defaultConfig;
            success &= ReportTest(
                defaultConfig.Width > 0 && defaultConfig.Height > 0,
                "GameConfig has a valid default window size"
            );

            if (success)
            {
                Logger::Info("Self-tests completed successfully.");
                return 0;
            }

            Logger::Error("One or more self-tests failed.");
            return 2;
        }

        void PrintRendererInfo()
        {
            constexpr RendererBackend backends[] = {
                RendererBackend::None,
                RendererBackend::DirectX,
                RendererBackend::Vulkan,
                RendererBackend::Metal
            };

            Logger::Info("Renderer backend availability:");

            for (const RendererBackend backend : backends)
            {
                Logger::Info(
                    std::string("  ") +
                    RendererBackendToString(backend) +
                    ": " +
                    (Renderer::IsBackendAvailable(backend) ? "available" : "not available")
                );
            }

            Logger::Info(
                std::string("Default renderer: ") +
                RendererBackendToString(Renderer::GetDefaultBackend())
            );
        }
    }

    int Application::Run()
    {
        return Run(nullptr, nullptr, 0, nullptr);
    }

    int Application::Run(int argc, char** argv)
    {
        return Run(nullptr, nullptr, argc, argv);
    }

    int Application::Run(
        Game* game,
        const GameConfig* config,
        int argc,
        char** argv
    )
    {
        CommandLineOptions options;
        std::string commandLineError;

        if (!ParseCommandLine(argc, argv, options, commandLineError))
        {
            Logger::Error(commandLineError);
            Logger::Info("Use --help to see available options.");
            return 2;
        }

        if (options.ShowHelp)
        {
            PrintCommandLineHelp();
            return 0;
        }

        if (options.ShowVersion)
        {
            std::cout
                << "Jevaing "
                << Jevaing::GetVersion()
                << " - "
                << Jevaing::GetCodename()
                << std::endl;
            return 0;
        }

        if (options.SelfTest)
        {
            return RunSelfTests();
        }

        if (options.AssetInfo)
        {
            return RunAssetInfo(options.AssetInfoPath);
        }

        if (options.AssetCacheTest)
        {
            return RunAssetCacheTest();
        }

        if (options.AssetErrorTest)
        {
            return RunAssetErrorTest();
        }

        if (options.ShowRendererInfo)
        {
            PrintRendererInfo();
            return 0;
        }

        const int graphicsTestCount =
            (options.GraphicsTest ? 1 : 0) +
            (options.PenguinGraphicsTest ? 1 : 0) +
            (options.GraphicsTest3D ? 1 : 0) +
            (options.PenguinTest3D ? 1 : 0) +
            (options.Gummy3DTest ? 1 : 0) +
            (options.ModelTest ? 1 : 0) +
            (options.TextureTest ? 1 : 0) +
            (options.MaterialTest ? 1 : 0) +
            (options.LightingTest ? 1 : 0) +
            (options.MultiModelTest ? 1 : 0) +
            (options.Mixed2D3DTest ? 1 : 0);

        if (graphicsTestCount > 1)
        {
            Logger::Error("Only one graphics test can be selected at a time.");
            return 2;
        }

        const bool graphicsTestRequested =
            options.GraphicsTest ||
            options.PenguinGraphicsTest ||
            options.GraphicsTest3D ||
            options.PenguinTest3D ||
            options.Gummy3DTest ||
            options.ModelTest ||
            options.TextureTest ||
            options.MaterialTest ||
            options.LightingTest ||
            options.MultiModelTest ||
            options.Mixed2D3DTest;

        if (options.RuntimeTest && graphicsTestRequested)
        {
            Logger::Error("--runtime-test cannot be combined with a graphics test.");
            return 2;
        }

        if (options.RuntimeTest && !game)
        {
            Logger::Error("--runtime-test requires a client Game instance.");
            return 2;
        }

        if (graphicsTestRequested && !options.HasFrameLimit)
        {
            options.HasFrameLimit = true;
            options.FrameLimit = 180;
        }

        if (options.RuntimeTest && !options.HasFrameLimit)
        {
            options.HasFrameLimit = true;
            options.FrameLimit = 120;
        }

        const std::string engineName =
            std::string("Jevaing ") + Jevaing::GetVersion() + " - " + Jevaing::GetCodename();

        const int windowWidth =
            config && config->Width > 0
                ? config->Width
                : 1280;

        const int windowHeight =
            config && config->Height > 0
                ? config->Height
                : 720;

        const std::string windowTitle =
            config && !config->Title.empty()
                ? config->Title
                : engineName;

        Logger::Info(engineName);
        Logger::Info("Initializing engine...");

        RendererBackend selectedBackend = Renderer::GetDefaultBackend();

        if (!options.Renderer.empty())
        {
            if (!RendererBackendFromString(options.Renderer, selectedBackend))
            {
                Logger::Error("Unknown renderer backend: " + options.Renderer);
                return 2;
            }
        }

        if (!Renderer::IsBackendAvailable(selectedBackend))
        {
            Logger::Error(
                std::string("Renderer backend is not available: ") +
                RendererBackendToString(selectedBackend)
            );
            return 2;
        }

        if (graphicsTestRequested && selectedBackend == RendererBackend::None)
        {
            Logger::Error("Graphics tests require a GPU-backed renderer.");
            return 2;
        }

        std::shared_ptr<const Model> primaryTestModel;
        std::shared_ptr<const Model> secondaryTestModel;
        std::shared_ptr<const Texture2D> testTexture;

        if (options.PenguinTest3D)
        {
            std::string loadError;
            if (!PrepareModelAsset(
                "geometry/3D/.hide/easter/tux.glb",
                primaryTestModel,
                loadError
            ))
            {
                Logger::Error(loadError);
                return 2;
            }
        }
        else if (options.Gummy3DTest)
        {
            std::string loadError;
            if (!PrepareModelAsset(
                "geometry/3D/.hide/easter/gummybear.fbx",
                primaryTestModel,
                loadError
            ))
            {
                Logger::Error(loadError);
                return 2;
            }
        }
        else if (options.ModelTest)
        {
            std::string loadError;
            if (!PrepareModelAsset(options.ModelTestPath, primaryTestModel, loadError))
            {
                Logger::Error(loadError);
                return 2;
            }
        }
        else if (options.MultiModelTest)
        {
            std::string loadError;
            if (!PrepareModelAsset(
                "geometry/3D/.hide/easter/tux.glb",
                primaryTestModel,
                loadError
            ))
            {
                Logger::Error(loadError);
                return 2;
            }

            if (!PrepareModelAsset(
                "geometry/3D/.hide/easter/gummybear.fbx",
                secondaryTestModel,
                loadError
            ))
            {
                Logger::Error(loadError);
                return 2;
            }
        }

        if (options.TextureTest || options.MaterialTest)
        {
            testTexture = Assets::CreateCheckerTexture();
        }

        WindowConfig windowConfig;
        windowConfig.Title = windowTitle;
        windowConfig.Width = windowWidth;
        windowConfig.Height = windowHeight;

        std::unique_ptr<Window> window = Window::Create(windowConfig);

        if (!window)
        {
            Logger::Error("Failed to create a platform window.");
            return 1;
        }

        window->Show();

        Logger::Info(
            "Window created: " +
            std::to_string(window->GetWidth()) +
            "x" +
            std::to_string(window->GetHeight())
        );

        RendererConfig rendererConfig;
        rendererConfig.Backend = selectedBackend;
        rendererConfig.TestModel = primaryTestModel;
        rendererConfig.SecondaryTestModel = secondaryTestModel;
        rendererConfig.TestTexture = testTexture;

        if (options.PenguinGraphicsTest)
        {
            rendererConfig.TestPattern = RendererTestPattern::Penguin;
        }
        else if (options.GraphicsTest3D)
        {
            rendererConfig.TestPattern = RendererTestPattern::Cube;
        }
        else if (options.PenguinTest3D || options.Gummy3DTest || options.ModelTest)
        {
            rendererConfig.TestPattern = RendererTestPattern::ExternalModel;
        }
        else if (options.TextureTest)
        {
            rendererConfig.TestPattern = RendererTestPattern::Texture;
        }
        else if (options.MaterialTest)
        {
            rendererConfig.TestPattern = RendererTestPattern::Material;
        }
        else if (options.LightingTest)
        {
            rendererConfig.TestPattern = RendererTestPattern::Lighting;
        }
        else if (options.MultiModelTest)
        {
            rendererConfig.TestPattern = RendererTestPattern::MultiModel;
        }
        else if (options.Mixed2D3DTest)
        {
            rendererConfig.TestPattern = RendererTestPattern::Mixed2D3D;
        }
        else if (options.GraphicsTest || !game)
        {
            rendererConfig.TestPattern = RendererTestPattern::Triangle;
        }
        else
        {
            rendererConfig.TestPattern = RendererTestPattern::None;
        }

        std::unique_ptr<Renderer> renderer = Renderer::Create(
            rendererConfig,
            *window
        );

        if (!renderer)
        {
            Logger::Error("Failed to create the renderer.");
            return 1;
        }

        Logger::Info(
            std::string("Renderer initialized: ") +
            renderer->GetName() +
            " [" +
            RendererBackendToString(renderer->GetBackend()) +
            "]"
        );

        if (options.PenguinGraphicsTest)
        {
            Logger::Info("BIG BEAR GUMMY 2D penguin graphics test enabled.");
        }
        else if (options.GraphicsTest3D)
        {
            Logger::Info("BIG BEAR GUMMY 3D cube graphics test enabled.");
        }
        else if (options.PenguinTest3D)
        {
            Logger::Info("External Tux GLB 3D test enabled.");
        }
        else if (options.Gummy3DTest)
        {
            Logger::Info("External gummy bear FBX 3D test enabled.");
        }
        else if (options.ModelTest)
        {
            Logger::Info("Generic model 3D test enabled: " + options.ModelTestPath);
        }
        else if (options.TextureTest)
        {
            Logger::Info("Texture2D test enabled.");
        }
        else if (options.MaterialTest)
        {
            Logger::Info("Material test enabled.");
        }
        else if (options.LightingTest)
        {
            Logger::Info("Directional lighting test enabled.");
        }
        else if (options.MultiModelTest)
        {
            Logger::Info("Multi-model asset test enabled.");
        }
        else if (options.Mixed2D3DTest)
        {
            Logger::Info("Mixed 2D + 3D test enabled.");
        }
        else if (options.GraphicsTest)
        {
            Logger::Info("BIG BEAR GUMMY triangle graphics test enabled.");
        }

        if (options.RuntimeTest)
        {
            Logger::Info("BIG BEAR GUMMY client runtime test enabled.");
        }

        if (options.HasFrameLimit)
        {
            Logger::Info(
                "Automatic frame limit enabled: " +
                std::to_string(options.FrameLimit)
            );
        }
        else
        {
            Logger::Info("Use WASD/arrows in the Sandbox. Press ESC to exit.");
        }

        const bool clientMode = game != nullptr && !graphicsTestRequested;
        bool gameStarted = false;

        int lastWidth = window->GetWidth();
        int lastHeight = window->GetHeight();

        if (clientMode)
        {
            InputState::Reset();
            game->OnStart();
            gameStarted = true;
            game->OnResize(lastWidth, lastHeight);
            Logger::Info("Client Game callbacks initialized.");
        }

        Timer timer;
        Logger::Info("Timer initialized.");
        Logger::Info("Engine initialized.");

        std::uint64_t frameCount = 0;
        std::uint64_t updateCalls = 0;
        std::uint64_t renderCalls = 0;
        int exitCode = 0;

        while (true)
        {
            InputState::BeginFrame();

            if (!window->ProcessEvents())
            {
                break;
            }

            const int currentWidth = window->GetWidth();
            const int currentHeight = window->GetHeight();
            const bool drawable = currentWidth > 0 && currentHeight > 0;

            if (
                drawable &&
                (currentWidth != lastWidth || currentHeight != lastHeight)
            )
            {
                if (!renderer->Resize(currentWidth, currentHeight))
                {
                    Logger::Error("Renderer resize failed.");
                    exitCode = 1;
                    break;
                }

                lastWidth = currentWidth;
                lastHeight = currentHeight;

                if (clientMode)
                {
                    game->OnResize(currentWidth, currentHeight);
                }
            }

            const double deltaTime = timer.Tick();

            if (clientMode)
            {
                game->OnUpdate(deltaTime);
                ++updateCalls;
            }

            if (drawable)
            {
                renderer->BeginFrame();

                if (clientMode)
                {
                    game->OnRender(static_cast<Graphics2D&>(*renderer));
                    game->OnRender(static_cast<Graphics3D&>(*renderer));
                    ++renderCalls;
                }

                renderer->EndFrame();
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }

            ++frameCount;

            if (options.HasFrameLimit && frameCount >= options.FrameLimit)
            {
                Logger::Info(
                    "Frame limit reached after " +
                    std::to_string(frameCount) +
                    " frames."
                );
                break;
            }

            if (renderer->GetBackend() == RendererBackend::None)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        if (gameStarted)
        {
            game->OnStop();
            InputState::Reset();
        }

        if (exitCode == 0 && options.PenguinGraphicsTest)
        {
            Logger::Info("[PASS] BIG BEAR GUMMY 2D penguin graphics test completed.");
        }
        else if (exitCode == 0 && options.GraphicsTest3D)
        {
            Logger::Info("[PASS] BIG BEAR GUMMY 3D cube graphics test completed.");
        }
        else if (exitCode == 0 && options.PenguinTest3D)
        {
            Logger::Info("[PASS] Tux GLB external-model test completed.");
        }
        else if (exitCode == 0 && options.Gummy3DTest)
        {
            Logger::Info("[PASS] Gummy bear FBX external-model test completed.");
        }
        else if (exitCode == 0 && options.ModelTest)
        {
            Logger::Info("[PASS] Generic model test completed.");
        }
        else if (exitCode == 0 && options.TextureTest)
        {
            Logger::Info("[PASS] Texture2D test completed.");
        }
        else if (exitCode == 0 && options.MaterialTest)
        {
            Logger::Info("[PASS] Material test completed.");
        }
        else if (exitCode == 0 && options.LightingTest)
        {
            Logger::Info("[PASS] Directional lighting test completed.");
        }
        else if (exitCode == 0 && options.MultiModelTest)
        {
            Logger::Info("[PASS] Multi-model test completed.");
        }
        else if (exitCode == 0 && options.Mixed2D3DTest)
        {
            Logger::Info("[PASS] Mixed 2D + 3D test completed.");
        }
        else if (exitCode == 0 && options.GraphicsTest)
        {
            Logger::Info("[PASS] BIG BEAR GUMMY graphics pipeline smoke test completed.");
        }

        if (exitCode == 0 && options.RuntimeTest)
        {
            if (updateCalls > 0 && renderCalls > 0)
            {
                Logger::Info(
                    "[PASS] BIG BEAR GUMMY client runtime callbacks completed."
                );
            }
            else
            {
                Logger::Error(
                    "[FAIL] BIG BEAR GUMMY client runtime callbacks were not exercised."
                );
                exitCode = 2;
            }
        }

        Logger::Info("Shutting down...");
        Logger::Info("Goodbye.");

        return exitCode;
    }
}

namespace Jevaing
{
    int Run()
    {
        Internal::Application application;
        return application.Run();
    }

    int Run(int argc, char** argv)
    {
        Internal::Application application;
        return application.Run(argc, argv);
    }

    int Run(Game& game, const GameConfig& config)
    {
        Internal::Application application;
        return application.Run(&game, &config, 0, nullptr);
    }

    int Run(Game& game, const GameConfig& config, int argc, char** argv)
    {
        Internal::Application application;
        return application.Run(&game, &config, argc, argv);
    }
}
