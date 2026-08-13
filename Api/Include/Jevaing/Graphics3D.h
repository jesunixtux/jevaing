#pragma once

#include "Assets.h"
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

        virtual void DrawMesh(
            const Mesh& mesh,
            const Transform& transform,
            const Material& material
        ) = 0;

        virtual void SetDirectionalLight(const DirectionalLight& light) = 0;
    };
}
