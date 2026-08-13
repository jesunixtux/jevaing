#pragma once

#include <string>

#include <Jevaing/Assets.h>

namespace Jevaing::Internal::Geometry3D
{
    class TextureLoader
    {
    public:
        static bool Load(
            const std::string& path,
            Texture2D& texture,
            std::string& error
        );
    };
}
