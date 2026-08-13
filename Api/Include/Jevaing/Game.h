#pragma once

#include <string>

#include "Graphics2D.h"
#include "Graphics3D.h"

namespace Jevaing
{
    struct GameConfig
    {
        std::string Title = "Jevaing";
        int Width = 1280;
        int Height = 720;
    };

    class Game
    {
    public:
        virtual ~Game() = default;

        virtual void OnStart()
        {
        }

        virtual void OnUpdate(double)
        {
        }

        virtual void OnRender(Graphics2D&)
        {
        }

        virtual void OnRender(Graphics3D&)
        {
        }

        virtual void OnResize(int, int)
        {
        }

        virtual void OnStop()
        {
        }
    };
}
