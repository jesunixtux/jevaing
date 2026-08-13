#pragma once

#include <cstddef>
#include <vector>

#include <Jevaing/Types.h>

namespace Jevaing::Internal::Geometry3D
{
    struct MeshVertex
    {
        Vec3 Position = {};
        Color VertexColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct Mesh
    {
        std::vector<MeshVertex> Vertices;

        bool Empty() const
        {
            return Vertices.empty();
        }

        std::size_t TriangleCount() const
        {
            return Vertices.size() / 3;
        }
    };
}
