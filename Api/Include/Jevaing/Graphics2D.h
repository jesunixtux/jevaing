#pragma once

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
    };
}
