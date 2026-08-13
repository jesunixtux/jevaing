#pragma once

#include <string>

#include "Mesh.h"

namespace Jevaing::Internal::Geometry3D
{
    struct ModelLoadOptions
    {
        bool CenterAndNormalize = true;
        float TargetExtent = 1.8f;
    };

    class ModelLoader
    {
    public:
        static bool Load(
            const std::string& path,
            Model& model,
            const ModelLoadOptions& options,
            std::string& error
        );

        static bool Load(
            const std::string& path,
            Mesh& mesh,
            const ModelLoadOptions& options,
            std::string& error
        );
    };
}
