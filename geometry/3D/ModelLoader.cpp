#include "ModelLoader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cctype>
#include <filesystem>
#include <limits>
#include <string>
#include <utility>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace Jevaing::Internal::Geometry3D
{
    namespace
    {
        bool IsSupportedModelExtension(const std::filesystem::path& path)
        {
            std::string extension = path.extension().string();
            std::transform(
                extension.begin(),
                extension.end(),
                extension.begin(),
                [](unsigned char character)
                {
                    return static_cast<char>(std::tolower(character));
                }
            );

            return
                extension == ".glb" ||
                extension == ".gltf" ||
                extension == ".fbx";
        }

        std::string FormatFromPath(const std::filesystem::path& path)
        {
            std::string extension = path.extension().string();

            if (!extension.empty() && extension[0] == '.')
            {
                extension.erase(extension.begin());
            }

            return extension;
        }

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

        Color ReadMaterialColor(const aiMaterial* material)
        {
            if (!material)
            {
                return { 0.72f, 0.78f, 0.86f, 1.0f };
            }

            aiColor4D baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };

            if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &baseColor) == AI_SUCCESS)
            {
                return { baseColor.r, baseColor.g, baseColor.b, baseColor.a };
            }

            if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &baseColor) == AI_SUCCESS)
            {
                return { baseColor.r, baseColor.g, baseColor.b, baseColor.a };
            }

            return { 0.72f, 0.78f, 0.86f, 1.0f };
        }

        std::string ReadMaterialName(const aiMaterial* material)
        {
            if (!material)
            {
                return {};
            }

            aiString name;

            if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS && name.length > 0)
            {
                return name.C_Str();
            }

            return {};
        }

        std::string ReadTexturePath(const aiMaterial* material)
        {
            if (!material)
            {
                return {};
            }

            aiString path;

            if (
                material->GetTexture(aiTextureType_BASE_COLOR, 0, &path) == AI_SUCCESS ||
                material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS
            )
            {
                return path.C_Str();
            }

            return {};
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

        Vec3 ReadNormal(const aiMesh* mesh, unsigned int vertexIndex)
        {
            if (!mesh || !mesh->HasNormals())
            {
                return { 0.0f, 1.0f, 0.0f };
            }

            const aiVector3D& sourceNormal = mesh->mNormals[vertexIndex];
            return Normalize({ sourceNormal.x, sourceNormal.y, sourceNormal.z });
        }

        Vec2 ReadUV0(const aiMesh* mesh, unsigned int vertexIndex)
        {
            if (!mesh || !mesh->HasTextureCoords(0))
            {
                return {};
            }

            const aiVector3D& sourceUV = mesh->mTextureCoords[0][vertexIndex];
            return { sourceUV.x, sourceUV.y };
        }

        void RecalculateModelBounds(Model& model)
        {
            model.Bounds = {};

            for (Mesh& mesh : model.Meshes)
            {
                mesh.Bounds = {};

                for (const Vertex3D& vertex : mesh.Vertices)
                {
                    ExpandBounds(mesh.Bounds, vertex.Position);
                    ExpandBounds(model.Bounds, vertex.Position);
                }
            }
        }

        void CenterAndNormalizeModel(Model& model, const ModelLoadOptions& options)
        {
            if (!model.Bounds.Valid)
            {
                return;
            }

            const Vec3 extent = {
                model.Bounds.Max.X - model.Bounds.Min.X,
                model.Bounds.Max.Y - model.Bounds.Min.Y,
                model.Bounds.Max.Z - model.Bounds.Min.Z
            };

            const float maximumExtent = std::max({ extent.X, extent.Y, extent.Z });

            if (!std::isfinite(maximumExtent) || maximumExtent <= 0.000001f)
            {
                return;
            }

            const Vec3 center = {
                (model.Bounds.Min.X + model.Bounds.Max.X) * 0.5f,
                (model.Bounds.Min.Y + model.Bounds.Max.Y) * 0.5f,
                (model.Bounds.Min.Z + model.Bounds.Max.Z) * 0.5f
            };

            const float targetExtent =
                options.TargetExtent > 0.000001f
                    ? options.TargetExtent
                    : 1.8f;

            const float scale = targetExtent / maximumExtent;

            for (Mesh& mesh : model.Meshes)
            {
                for (Vertex3D& vertex : mesh.Vertices)
                {
                    vertex.Position = {
                        (vertex.Position.X - center.X) * scale,
                        (vertex.Position.Y - center.Y) * scale,
                        (vertex.Position.Z - center.Z) * scale
                    };
                }
            }

            RecalculateModelBounds(model);
        }
    }

    bool ModelLoader::Load(
        const std::string& path,
        Model& model,
        const ModelLoadOptions& options,
        std::string& error
    )
    {
        model = {};
        error.clear();

        const std::filesystem::path modelPath(path);

        if (!IsSupportedModelExtension(modelPath))
        {
            error =
                "Unsupported model format for asset: " +
                path +
                " (supported: .glb, .gltf, .fbx)";
            return false;
        }

        Assimp::Importer importer;
        const unsigned int flags =
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenSmoothNormals |
            aiProcess_PreTransformVertices |
            aiProcess_SortByPType |
            aiProcess_ValidateDataStructure |
            aiProcess_ConvertToLeftHanded |
            aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFile(path, flags);

        if (!scene || !scene->HasMeshes())
        {
            error = "Assimp could not load model: " + path;

            const char* detail = importer.GetErrorString();
            if (detail && *detail)
            {
                error += " (Assimp: ";
                error += detail;
                error += ")";
            }

            return false;
        }

        model.SourcePath = path;
        model.Format = FormatFromPath(modelPath);

        model.Materials.reserve(scene->mNumMaterials > 0 ? scene->mNumMaterials : 1);

        for (unsigned int materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
        {
            const aiMaterial* sourceMaterial = scene->mMaterials[materialIndex];

            Material material;
            material.Name = ReadMaterialName(sourceMaterial);
            material.BaseColor = ReadMaterialColor(sourceMaterial);
            material.BaseColorTexturePath = ReadTexturePath(sourceMaterial);
            model.Materials.push_back(material);
        }

        if (model.Materials.empty())
        {
            model.Materials.push_back({});
        }

        model.Meshes.reserve(scene->mNumMeshes);

        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh* sourceMesh = scene->mMeshes[meshIndex];

            if (!sourceMesh || !sourceMesh->HasPositions())
            {
                continue;
            }

            Mesh mesh;
            mesh.Name = sourceMesh->mName.length > 0 ? sourceMesh->mName.C_Str() : "";
            mesh.MaterialIndex =
                sourceMesh->mMaterialIndex < model.Materials.size()
                    ? sourceMesh->mMaterialIndex
                    : 0;
            mesh.HasNormals = sourceMesh->HasNormals();
            mesh.HasUV0 = sourceMesh->HasTextureCoords(0);
            mesh.Vertices.reserve(sourceMesh->mNumVertices);
            mesh.Indices.reserve(sourceMesh->mNumFaces * 3);

            for (unsigned int vertexIndex = 0; vertexIndex < sourceMesh->mNumVertices; ++vertexIndex)
            {
                const aiVector3D& sourcePosition = sourceMesh->mVertices[vertexIndex];
                const Vec3 position = {
                    sourcePosition.x,
                    sourcePosition.y,
                    sourcePosition.z
                };

                mesh.Vertices.push_back({
                    position,
                    ReadNormal(sourceMesh, vertexIndex),
                    ReadUV0(sourceMesh, vertexIndex),
                    ReadVertexColor(
                        sourceMesh,
                        vertexIndex,
                        { 1.0f, 1.0f, 1.0f, 1.0f }
                    )
                });

                ExpandBounds(mesh.Bounds, position);
                ExpandBounds(model.Bounds, position);
            }

            for (unsigned int faceIndex = 0; faceIndex < sourceMesh->mNumFaces; ++faceIndex)
            {
                const aiFace& face = sourceMesh->mFaces[faceIndex];

                if (face.mNumIndices != 3)
                {
                    continue;
                }

                for (unsigned int corner = 0; corner < 3; ++corner)
                {
                    if (face.mIndices[corner] >= sourceMesh->mNumVertices)
                    {
                        error = "Model contains an invalid vertex index: " + path;
                        model = {};
                        return false;
                    }

                    mesh.Indices.push_back(static_cast<std::uint32_t>(face.mIndices[corner]));
                }
            }

            if (!mesh.Empty())
            {
                model.Meshes.push_back(std::move(mesh));
            }
        }

        if (model.Empty())
        {
            error = "Model contains no triangle geometry: " + path;
            return false;
        }

        if (options.CenterAndNormalize)
        {
            if (!model.Bounds.Valid)
            {
                error = "Model has invalid bounds: " + path;
                model = {};
                return false;
            }

            CenterAndNormalizeModel(model, options);
        }

        return true;
    }

    bool ModelLoader::Load(
        const std::string& path,
        Mesh& mesh,
        const ModelLoadOptions& options,
        std::string& error
    )
    {
        Model model;

        if (!Load(path, model, options, error))
        {
            mesh = {};
            return false;
        }

        if (model.Meshes.size() == 1)
        {
            mesh = model.Meshes.front();
            return true;
        }

        mesh = {};
        mesh.Name = "Combined Model Mesh";

        for (const Mesh& sourceMesh : model.Meshes)
        {
            const std::uint32_t baseVertex =
                static_cast<std::uint32_t>(mesh.Vertices.size());

            mesh.Vertices.insert(
                mesh.Vertices.end(),
                sourceMesh.Vertices.begin(),
                sourceMesh.Vertices.end()
            );

            for (std::uint32_t index : sourceMesh.Indices)
            {
                mesh.Indices.push_back(baseVertex + index);
            }

            mesh.HasNormals = mesh.HasNormals || sourceMesh.HasNormals;
            mesh.HasUV0 = mesh.HasUV0 || sourceMesh.HasUV0;
        }

        RecalculateModelBounds(model);
        mesh.Bounds = model.Bounds;
        return !mesh.Empty();
    }
}
