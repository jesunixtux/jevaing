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

        Platform::WindowsWindow window;

        if (!window.Create(
            L"Jevaing 0.0.1 - RENACO",
            1280,
            720
        ))
        {
            std::cerr
                << "[Jevaing] Failed to create Windows window."
                << std::endl;

            return 1;
        }

        window.Show();

        std::cout
            << "[Jevaing] Window created."
            << std::endl;

        std::cout
            << "[Jevaing] Engine initialized."
            << std::endl;

        while (window.ProcessMessages())
        {
            // Futuro:
            //
            // Input
            // Update
            // Render
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