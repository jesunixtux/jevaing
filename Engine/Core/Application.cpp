#include <iostream>

#include <Jevaing/Jevaing.h>

#include "Application.h"

namespace Jevaing::Internal
{
    int Application::Run()
    {
        std::cout << "==============================" << std::endl;
        std::cout << "       Jevaing Engine" << std::endl;
        std::cout << "==============================" << std::endl;

        std::cout << "Version:  " << Jevaing::GetVersion() << std::endl;
        std::cout << "Codename: " << Jevaing::GetCodename() << std::endl;

        std::cout << std::endl;
        std::cout << "[Jevaing] Initializing engine..." << std::endl;

        // RENACO 0.0.1:
        // Aqui iran apareciendo los primeros sistemas.

        std::cout << "[Jevaing] Engine initialized." << std::endl;

        std::cout << std::endl;
        std::cout << "[Jevaing] Shutting down..." << std::endl;
        std::cout << "[Jevaing] Goodbye." << std::endl;

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
