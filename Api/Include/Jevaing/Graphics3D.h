#pragma once

#include "Assets.h"
#include "Types.h"

namespace Jevaing
{
    struct Skybox
    {
        Color Zenith = { 0.06f, 0.18f, 0.42f, 1.0f };
        Color Horizon = { 0.55f, 0.72f, 0.90f, 1.0f };
        Color Nadir = { 0.12f, 0.16f, 0.24f, 1.0f };
    };

    class Graphics3D
    {
    public:
        virtual ~Graphics3D() = default;

        virtual void SetCamera(const PerspectiveCamera& camera) = 0;
        virtual void DrawSkybox(const Skybox& skybox) = 0;

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
