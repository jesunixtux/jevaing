#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include <Jevaing/Jevaing.h>

#include "Application.h"
#include "CommandLine.h"
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
        return Run(0, nullptr);
    }

    int Application::Run(int argc, char** argv)
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

        constexpr int WindowWidth = 1280;
        constexpr int WindowHeight = 720;

        const std::string engineName =
            std::string("Jevaing ") + Jevaing::GetVersion() + " - " + Jevaing::GetCodename();

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

        WindowConfig windowConfig;
        windowConfig.Title = engineName;
        windowConfig.Width = WindowWidth;
        windowConfig.Height = WindowHeight;

        std::unique_ptr<Window> window = Window::Create(windowConfig);

        if (!window)
        {
            Logger::Error("Failed to create a platform window.");
            return 1;
        }

        window->Show();

        Logger::Info(
            "Window created: " +
            std::to_string(WindowWidth) +
            "x" +
            std::to_string(WindowHeight)
        );

        RendererConfig rendererConfig;
        rendererConfig.Backend = selectedBackend;

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

        if (options.HasFrameLimit)
        {
            Logger::Info(
                "Automatic frame limit enabled: " +
                std::to_string(options.FrameLimit)
            );
        }
        else
        {
            Logger::Info("Press ESC or close the window to exit.");
        }

        Timer timer;
        Logger::Info("Timer initialized.");
        Logger::Info("Engine initialized.");

        std::uint64_t frameCount = 0;

        while (window->ProcessEvents())
        {
            const double deltaTime = timer.Tick();
            (void)deltaTime;

            renderer->BeginFrame();

            // ARPA+ 0.0.4: the frame now reaches a real GPU backend on Windows.

            renderer->EndFrame();
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

        Logger::Info("Shutting down...");
        Logger::Info("Goodbye.");

        return 0;
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
}
