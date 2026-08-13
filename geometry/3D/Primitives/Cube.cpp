#include "Cube.h"

namespace Jevaing::Internal::Geometry3D
{
    namespace
    {
        Color Shade(float factor)
        {
            return { factor, factor, factor, 1.0f };
        }

        void AppendVertex(Mesh& mesh, const Vec3& position, const Color& color)
        {
            mesh.Vertices.push_back({ position, color });
        }

        void AppendQuad(
            Mesh& mesh,
            const Vec3& a,
            const Vec3& b,
            const Vec3& c,
            const Vec3& d,
            const Color& color
        )
        {
            AppendVertex(mesh, a, color);
            AppendVertex(mesh, b, color);
            AppendVertex(mesh, c, color);
            AppendVertex(mesh, a, color);
            AppendVertex(mesh, c, color);
            AppendVertex(mesh, d, color);
        }
    }

    Mesh CreateCubeMesh()
    {
        Mesh mesh;
        mesh.Vertices.reserve(36);

        constexpr float HalfSize = 0.5f;

        const Vec3 frontTopLeft = { -HalfSize, HalfSize, -HalfSize };
        const Vec3 frontTopRight = { HalfSize, HalfSize, -HalfSize };
        const Vec3 frontBottomRight = { HalfSize, -HalfSize, -HalfSize };
        const Vec3 frontBottomLeft = { -HalfSize, -HalfSize, -HalfSize };

        const Vec3 backTopLeft = { -HalfSize, HalfSize, HalfSize };
        const Vec3 backTopRight = { HalfSize, HalfSize, HalfSize };
        const Vec3 backBottomRight = { HalfSize, -HalfSize, HalfSize };
        const Vec3 backBottomLeft = { -HalfSize, -HalfSize, HalfSize };

        AppendQuad(
            mesh,
            frontTopLeft,
            frontTopRight,
            frontBottomRight,
            frontBottomLeft,
            Shade(1.00f)
        );

        AppendQuad(
            mesh,
            backTopRight,
            backTopLeft,
            backBottomLeft,
            backBottomRight,
            Shade(0.58f)
        );

        AppendQuad(
            mesh,
            backTopLeft,
            frontTopLeft,
            frontBottomLeft,
            backBottomLeft,
            Shade(0.72f)
        );

        AppendQuad(
            mesh,
            frontTopRight,
            backTopRight,
            backBottomRight,
            frontBottomRight,
            Shade(0.84f)
        );

        AppendQuad(
            mesh,
            backTopLeft,
            backTopRight,
            frontTopRight,
            frontTopLeft,
            Shade(1.12f)
        );

        AppendQuad(
            mesh,
            frontBottomLeft,
            frontBottomRight,
            backBottomRight,
            backBottomLeft,
            Shade(0.46f)
        );

        return mesh;
    }
}
