#include <chrono>
#include <cstdint>
#include <iostream>
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

        if (options.ShowRendererInfo)
        {
            PrintRendererInfo();
            return 0;
        }

        if (options.GraphicsTest && options.PenguinGraphicsTest)
        {
            Logger::Error(
                "--graphics-test and --graphics-test-penguin cannot be used together."
            );
            return 2;
        }

        const bool graphicsTestRequested =
            options.GraphicsTest || options.PenguinGraphicsTest;

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

        if (options.PenguinGraphicsTest)
        {
            rendererConfig.TestPattern = RendererTestPattern::Penguin;
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
            Logger::Info("BIG BEAR GUMMY penguin graphics test enabled.");
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
                    game->OnRender(*renderer);
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
            Logger::Info("[PASS] BIG BEAR GUMMY penguin graphics test completed.");
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
