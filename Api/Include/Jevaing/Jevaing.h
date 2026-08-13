#pragma once

#include "Game.h"
#include "Graphics2D.h"
#include "Input.h"
#include "Types.h"

namespace Jevaing
{
    const char* GetVersion();
    const char* GetCodename();

    int Run();
    int Run(int argc, char** argv);

    int Run(Game& game, const GameConfig& config);
    int Run(Game& game, const GameConfig& config, int argc, char** argv);
}
