#pragma once

#include <memory>

#include "Assets.h"
#include "Types.h"

namespace Jevaing
{
    class Graphics2D
    {
    public:
        virtual ~Graphics2D() = default;

        virtual void Clear(const Color& color) = 0;

        virtual void DrawTriangle(
            const Vec2& a,
            const Vec2& b,
            const Vec2& c,
            const Color& color
        ) = 0;

        virtual void DrawQuad(
            const Vec2& center,
            const Vec2& size,
            const Color& color
        ) = 0;

        virtual void DrawSprite(
            const std::shared_ptr<const Texture2D>& texture,
            const Vec2& center,
            const Vec2& size,
            const Color& tint
        ) = 0;
    };
}
