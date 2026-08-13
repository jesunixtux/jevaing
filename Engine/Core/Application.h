#pragma once

namespace Jevaing::Internal
{
    class Application
    {
    public:
        Application() = default;
        ~Application() = default;

        int Run();
        int Run(int argc, char** argv);
    };
}
