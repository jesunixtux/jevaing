#pragma once

#include "Types.h"

namespace Jevaing
{
    class Graphics3D
    {
    public:
        virtual ~Graphics3D() = default;

        virtual void SetCamera(const PerspectiveCamera& camera) = 0;

        virtual void DrawCube(
            const Transform& transform,
            const Color& color
        ) = 0;
    };
}
