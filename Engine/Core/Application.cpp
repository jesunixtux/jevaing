#include <iostream>

#include <Jevaing/Jevaing.h>

#include "Application.h"

#ifdef _WIN32
#include "../Platform/Windows/WindowsWindow.h"
#endif

namespace Jevaing::Internal
{
    int Application::Run()
    {
        constexpr int WindowWidth = 1280;
        constexpr int WindowHeight = 720;

        std::cout
            << "=============================="
            << std::endl;

        std::cout
            << "       Jevaing Engine"
            << std::endl;

        std::cout
            << "=============================="
            << std::endl;

        std::cout
            << "Version:  "
            << Jevaing::GetVersion()
            << std::endl;

        std::cout
            << "Codename: "
            << Jevaing::GetCodename()
            << std::endl;

        std::cout << std::endl;

        std::cout
            << "[Jevaing] Initializing engine..."
            << std::endl;

#ifdef _WIN32

        std::cout
            << "[Jevaing] Platform: Windows (Win32)"
            << std::endl;

        Platform::WindowsWindow window;

        if (!window.Create(
            L"Jevaing 0.0.1 - RENACO",
            WindowWidth,
            WindowHeight
        ))
        {
            std::cerr
                << "[Jevaing] Failed to create Windows window."
                << std::endl;

            return 1;
        }

        window.Show();

        std::cout
            << "[Jevaing] Window created: "
            << WindowWidth
            << "x"
            << WindowHeight
            << std::endl;

        std::cout
            << "[Jevaing] Press ESC or close the window to exit."
            << std::endl;

        std::cout
            << "[Jevaing] Engine initialized."
            << std::endl;

        while (window.ProcessMessages())
        {
            // RENACO 0.0.1: basic native event loop.
        }

#else

        std::cerr
            << "[Jevaing] Platform not implemented."
            << std::endl;

        return 1;

#endif

        std::cout
            << "[Jevaing] Shutting down..."
            << std::endl;

        std::cout
            << "[Jevaing] Goodbye."
            << std::endl;

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
