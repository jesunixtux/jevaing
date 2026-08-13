#include <chrono>
#include <string>
#include <thread>

#include <Jevaing/Jevaing.h>

#include "Application.h"
#include "Logger.h"
#include "Timer.h"
#include "Window.h"

namespace Jevaing::Internal
{
    int Application::Run()
    {
        constexpr int WindowWidth = 1280;
        constexpr int WindowHeight = 720;

        const std::string engineName =
            std::string("Jevaing ") + Jevaing::GetVersion() + " - " + Jevaing::GetCodename();

        Logger::Info(engineName);
        Logger::Info("Initializing engine...");

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
        Logger::Info("Press ESC or close the window to exit.");

        Timer timer;
        Logger::Info("Timer initialized.");
        Logger::Info("Engine initialized.");

        while (window->ProcessEvents())
        {
            const double deltaTime = timer.Tick();
            (void)deltaTime;

            // MARIA 0.0.2: basic engine loop foundation.
            // Future systems will consume deltaTime here.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
}
