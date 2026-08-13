#include "ModelLoader.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace Jevaing::Internal::Geometry3D
{
    namespace
    {
        Color ReadMaterialColor(const aiScene* scene, const aiMesh* mesh)
        {
            Color result = { 0.72f, 0.78f, 0.86f, 1.0f };

            if (!scene || !mesh || mesh->mMaterialIndex >= scene->mNumMaterials)
            {
                return result;
            }

            aiColor4D diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
            const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            if (
                material &&
                aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse) == AI_SUCCESS
            )
            {
                result = { diffuse.r, diffuse.g, diffuse.b, diffuse.a };
            }

            return result;
        }

        Color ReadVertexColor(
            const aiMesh* mesh,
            unsigned int vertexIndex,
            const Color& fallback
        )
        {
            if (!mesh || !mesh->HasVertexColors(0))
            {
                return fallback;
            }

            const aiColor4D& color = mesh->mColors[0][vertexIndex];
            return { color.r, color.g, color.b, color.a };
        }

        float CalculateLighting(const aiMesh* mesh, unsigned int vertexIndex)
        {
            if (!mesh || !mesh->HasNormals())
            {
                return 1.0f;
            }

            const aiVector3D& sourceNormal = mesh->mNormals[vertexIndex];
            const float normalLength = std::sqrt(
                sourceNormal.x * sourceNormal.x +
                sourceNormal.y * sourceNormal.y +
                sourceNormal.z * sourceNormal.z
            );

            if (normalLength <= 0.000001f)
            {
                return 1.0f;
            }

            constexpr float lightX = 0.350878f;
            constexpr float lightY = 0.852132f;
            constexpr float lightZ = -0.401003f;

            const float normalX = sourceNormal.x / normalLength;
            const float normalY = sourceNormal.y / normalLength;
            const float normalZ = sourceNormal.z / normalLength;

            const float diffuse = std::max(
                0.0f,
                normalX * lightX + normalY * lightY + normalZ * lightZ
            );

            return 0.38f + diffuse * 0.62f;
        }

        Color ApplyLighting(const Color& color, float lighting)
        {
            return {
                color.R * lighting,
                color.G * lighting,
                color.B * lighting,
                color.A
            };
        }
    }

    bool ModelLoader::Load(
        const std::string& path,
        Mesh& mesh,
        const ModelLoadOptions& options,
        std::string& error
    )
    {
        mesh.Vertices.clear();
        error.clear();

        Assimp::Importer importer;
        const unsigned int flags =
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenSmoothNormals |
            aiProcess_PreTransformVertices |
            aiProcess_SortByPType |
            aiProcess_ValidateDataStructure |
            aiProcess_ConvertToLeftHanded;

        const aiScene* scene = importer.ReadFile(path, flags);

        if (!scene || !scene->HasMeshes())
        {
            error = "Assimp could not load model: " + path;

            const char* detail = importer.GetErrorString();
            if (detail && *detail)
            {
                error += " (";
                error += detail;
                error += ")";
            }

            return false;
        }

        const float infinity = std::numeric_limits<float>::infinity();
        Vec3 boundsMin = { infinity, infinity, infinity };
        Vec3 boundsMax = { -infinity, -infinity, -infinity };

        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh* sourceMesh = scene->mMeshes[meshIndex];

            if (!sourceMesh || !sourceMesh->HasPositions())
            {
                continue;
            }

            const Color materialColor = ReadMaterialColor(scene, sourceMesh);

            for (unsigned int faceIndex = 0; faceIndex < sourceMesh->mNumFaces; ++faceIndex)
            {
                const aiFace& face = sourceMesh->mFaces[faceIndex];

                if (face.mNumIndices != 3)
                {
                    continue;
                }

                for (unsigned int corner = 0; corner < 3; ++corner)
                {
                    const unsigned int vertexIndex = face.mIndices[corner];

                    if (vertexIndex >= sourceMesh->mNumVertices)
                    {
                        error = "Model contains an invalid vertex index: " + path;
                        mesh.Vertices.clear();
                        return false;
                    }

                    const aiVector3D& sourcePosition = sourceMesh->mVertices[vertexIndex];
                    const Vec3 position = {
                        sourcePosition.x,
                        sourcePosition.y,
                        sourcePosition.z
                    };

                    boundsMin.X = std::min(boundsMin.X, position.X);
                    boundsMin.Y = std::min(boundsMin.Y, position.Y);
                    boundsMin.Z = std::min(boundsMin.Z, position.Z);
                    boundsMax.X = std::max(boundsMax.X, position.X);
                    boundsMax.Y = std::max(boundsMax.Y, position.Y);
                    boundsMax.Z = std::max(boundsMax.Z, position.Z);

                    const Color vertexColor = ReadVertexColor(
                        sourceMesh,
                        vertexIndex,
                        materialColor
                    );

                    mesh.Vertices.push_back({
                        position,
                        ApplyLighting(
                            vertexColor,
                            CalculateLighting(sourceMesh, vertexIndex)
                        )
                    });
                }
            }
        }

        if (mesh.Empty())
        {
            error = "Model contains no triangle geometry: " + path;
            return false;
        }

        if (options.CenterAndNormalize)
        {
            const Vec3 extent = {
                boundsMax.X - boundsMin.X,
                boundsMax.Y - boundsMin.Y,
                boundsMax.Z - boundsMin.Z
            };

            const float maximumExtent = std::max({ extent.X, extent.Y, extent.Z });

            if (!std::isfinite(maximumExtent) || maximumExtent <= 0.000001f)
            {
                error = "Model has invalid bounds: " + path;
                mesh.Vertices.clear();
                return false;
            }

            const Vec3 center = {
                (boundsMin.X + boundsMax.X) * 0.5f,
                (boundsMin.Y + boundsMax.Y) * 0.5f,
                (boundsMin.Z + boundsMax.Z) * 0.5f
            };

            const float targetExtent =
                options.TargetExtent > 0.000001f
                    ? options.TargetExtent
                    : 1.8f;

            const float scale = targetExtent / maximumExtent;

            for (MeshVertex& vertex : mesh.Vertices)
            {
                vertex.Position = {
                    (vertex.Position.X - center.X) * scale,
                    (vertex.Position.Y - center.Y) * scale,
                    (vertex.Position.Z - center.Z) * scale
                };
            }
        }

        return true;
    }
}
