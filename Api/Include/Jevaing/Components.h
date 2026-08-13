#pragma once

#include <memory>
#include <string>

#include "Assets.h"
#include "Types.h"

namespace Jevaing
{
    struct TransformComponent
    {
        Transform LocalTransform;
    };

    struct CameraComponent
    {
        PerspectiveCamera Camera;
        bool Primary = false;
    };

    struct MeshRendererComponent
    {
        std::string ModelPath;
        std::shared_ptr<const Model> ModelAsset;
        Material MaterialOverride;
        bool HasMaterialOverride = false;
    };

    struct SpriteRenderer2DComponent
    {
        std::string TexturePath;
        std::shared_ptr<const Texture2D> Texture;
        Vec2 Size = { 0.25f, 0.25f };
        Color Tint = { 1.0f, 1.0f, 1.0f, 1.0f };
        int Layer = 0;
    };
}
