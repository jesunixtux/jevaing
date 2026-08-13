#include "Cube.h"

#include <algorithm>
#include <cstdint>

namespace Jevaing::Internal::Geometry3D
{
    namespace
    {
        void ExpandBounds(Bounds3D& bounds, const Vec3& position)
        {
            if (!bounds.Valid)
            {
                bounds.Min = position;
                bounds.Max = position;
                bounds.Valid = true;
                return;
            }

            bounds.Min.X = std::min(bounds.Min.X, position.X);
            bounds.Min.Y = std::min(bounds.Min.Y, position.Y);
            bounds.Min.Z = std::min(bounds.Min.Z, position.Z);
            bounds.Max.X = std::max(bounds.Max.X, position.X);
            bounds.Max.Y = std::max(bounds.Max.Y, position.Y);
            bounds.Max.Z = std::max(bounds.Max.Z, position.Z);
        }

        void AppendFace(
            Mesh& mesh,
            const Vec3& a,
            const Vec3& b,
            const Vec3& c,
            const Vec3& d,
            const Vec3& normal
        )
        {
            const std::uint32_t start = static_cast<std::uint32_t>(mesh.Vertices.size());

            mesh.Vertices.push_back({ a, normal, { 0.0f, 0.0f } });
            mesh.Vertices.push_back({ b, normal, { 1.0f, 0.0f } });
            mesh.Vertices.push_back({ c, normal, { 1.0f, 1.0f } });
            mesh.Vertices.push_back({ d, normal, { 0.0f, 1.0f } });

            mesh.Indices.push_back(start + 0);
            mesh.Indices.push_back(start + 1);
            mesh.Indices.push_back(start + 2);
            mesh.Indices.push_back(start + 0);
            mesh.Indices.push_back(start + 2);
            mesh.Indices.push_back(start + 3);

            ExpandBounds(mesh.Bounds, a);
            ExpandBounds(mesh.Bounds, b);
            ExpandBounds(mesh.Bounds, c);
            ExpandBounds(mesh.Bounds, d);
        }
    }

    Mesh CreateCubeMesh()
    {
        Mesh mesh;
        mesh.Name = "Jevaing Cube";
        mesh.Vertices.reserve(24);
        mesh.Indices.reserve(36);
        mesh.HasNormals = true;
        mesh.HasUV0 = true;

        constexpr float HalfSize = 0.5f;

        const Vec3 frontTopLeft = { -HalfSize, HalfSize, -HalfSize };
        const Vec3 frontTopRight = { HalfSize, HalfSize, -HalfSize };
        const Vec3 frontBottomRight = { HalfSize, -HalfSize, -HalfSize };
        const Vec3 frontBottomLeft = { -HalfSize, -HalfSize, -HalfSize };

        const Vec3 backTopLeft = { -HalfSize, HalfSize, HalfSize };
        const Vec3 backTopRight = { HalfSize, HalfSize, HalfSize };
        const Vec3 backBottomRight = { HalfSize, -HalfSize, HalfSize };
        const Vec3 backBottomLeft = { -HalfSize, -HalfSize, HalfSize };

        AppendFace(
            mesh,
            frontTopLeft,
            frontTopRight,
            frontBottomRight,
            frontBottomLeft,
            { 0.0f, 0.0f, -1.0f }
        );

        AppendFace(
            mesh,
            backTopRight,
            backTopLeft,
            backBottomLeft,
            backBottomRight,
            { 0.0f, 0.0f, 1.0f }
        );

        AppendFace(
            mesh,
            backTopLeft,
            frontTopLeft,
            frontBottomLeft,
            backBottomLeft,
            { -1.0f, 0.0f, 0.0f }
        );

        AppendFace(
            mesh,
            frontTopRight,
            backTopRight,
            backBottomRight,
            frontBottomRight,
            { 1.0f, 0.0f, 0.0f }
        );

        AppendFace(
            mesh,
            backTopLeft,
            backTopRight,
            frontTopRight,
            frontTopLeft,
            { 0.0f, 1.0f, 0.0f }
        );

        AppendFace(
            mesh,
            frontBottomLeft,
            frontBottomRight,
            backBottomRight,
            backBottomLeft,
            { 0.0f, -1.0f, 0.0f }
        );

        return mesh;
    }

    Mesh CreatePlaneMesh()
    {
        Mesh mesh;
        mesh.Name = "Jevaing Plane";
        mesh.Vertices.reserve(4);
        mesh.Indices.reserve(6);
        mesh.HasNormals = true;
        mesh.HasUV0 = true;

        constexpr float HalfSize = 0.5f;
        AppendFace(
            mesh,
            { -HalfSize, 0.0f, -HalfSize },
            { HalfSize, 0.0f, -HalfSize },
            { HalfSize, 0.0f, HalfSize },
            { -HalfSize, 0.0f, HalfSize },
            { 0.0f, 1.0f, 0.0f }
        );

        return mesh;
    }
}
