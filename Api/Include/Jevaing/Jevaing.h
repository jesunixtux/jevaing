#pragma once

#include "Assets.h"
#include "Components.h"
#include "Entity.h"
#include "Game.h"
#include "Graphics2D.h"
#include "Graphics3D.h"
#include "Input.h"
#include "Types.h"
#include "Project.h"
#include "Scene.h"

namespace Jevaing
{
    const char* GetVersion();
    const char* GetCodename();

    int Run();
    int Run(int argc, char** argv);

    int Run(Game& game, const GameConfig& config);
    int Run(Game& game, const GameConfig& config, int argc, char** argv);
}
