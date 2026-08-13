#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Types.h"

namespace Jevaing
{
    enum class PixelFormat
    {
        Unknown,
        Rgba8
    };

    struct Texture2D
    {
        std::string SourcePath;
        int Width = 0;
        int Height = 0;
        int Channels = 0;
        PixelFormat Format = PixelFormat::Unknown;
        std::vector<std::uint8_t> Pixels;

        bool Empty() const
        {
            return Width <= 0 || Height <= 0 || Pixels.empty();
        }
    };

    struct Material
    {
        std::string Name;
        Color BaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        std::string BaseColorTexturePath;
        std::shared_ptr<const Texture2D> BaseColorTexture;
    };

    struct Bounds3D
    {
        Vec3 Min = {};
        Vec3 Max = {};
        bool Valid = false;
    };

    struct Vertex3D
    {
        Vec3 Position = {};
        Vec3 Normal = { 0.0f, 1.0f, 0.0f };
        Vec2 UV = {};
        Color VertexColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct Mesh
    {
        std::string Name;
        std::vector<Vertex3D> Vertices;
        std::vector<std::uint32_t> Indices;
        std::uint32_t MaterialIndex = 0;
        Bounds3D Bounds;
        bool HasNormals = false;
        bool HasUV0 = false;

        bool Empty() const
        {
            return Vertices.empty() || Indices.empty();
        }

        std::size_t IndexCount() const
        {
            return Indices.size();
        }

        std::size_t TriangleCount() const
        {
            return Indices.size() / 3;
        }
    };

    struct Model
    {
        std::string SourcePath;
        std::string Format;
        std::vector<Mesh> Meshes;
        std::vector<Material> Materials;
        Bounds3D Bounds;

        bool Empty() const
        {
            return Meshes.empty();
        }

        std::size_t VertexCount() const
        {
            std::size_t count = 0;

            for (const Mesh& mesh : Meshes)
            {
                count += mesh.Vertices.size();
            }

            return count;
        }

        std::size_t IndexCount() const
        {
            std::size_t count = 0;

            for (const Mesh& mesh : Meshes)
            {
                count += mesh.Indices.size();
            }

            return count;
        }

        std::size_t TriangleCount() const
        {
            return IndexCount() / 3;
        }

        bool HasNormals() const
        {
            for (const Mesh& mesh : Meshes)
            {
                if (mesh.HasNormals)
                {
                    return true;
                }
            }

            return false;
        }

        bool HasUV0() const
        {
            for (const Mesh& mesh : Meshes)
            {
                if (mesh.HasUV0)
                {
                    return true;
                }
            }

            return false;
        }
    };

    struct DirectionalLight
    {
        Vec3 Direction = { -0.35f, -0.85f, 0.40f };
        Color Color = { 1.0f, 0.96f, 0.88f, 1.0f };
        float Intensity = 1.0f;
    };

    namespace Assets
    {
        std::shared_ptr<const Model> LoadModel(
            const std::string& path,
            std::string* error = nullptr
        );

        std::shared_ptr<const Texture2D> LoadTexture2D(
            const std::string& path,
            std::string* error = nullptr
        );

        std::shared_ptr<const Texture2D> CreateCheckerTexture(
            int width = 128,
            int height = 128,
            int cellSize = 16
        );

        void ClearCache();

        std::size_t GetModelImportCountForPath(const std::string& path);
    }
}
