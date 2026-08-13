#pragma once

namespace Jevaing
{
    class Game;
    struct GameConfig;
}

namespace Jevaing::Internal
{
    class Application
    {
    public:
        Application() = default;
        ~Application() = default;

        int Run();
        int Run(int argc, char** argv);
        int Run(Game* game, const GameConfig* config, int argc, char** argv);
    };
}
