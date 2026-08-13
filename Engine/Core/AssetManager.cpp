#include <Jevaing/Assets.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include <geometry/3D/ModelLoader.h>
#include <geometry/3D/TextureLoader.h>

#include "Logger.h"

namespace Jevaing
{
    namespace
    {
        namespace fs = std::filesystem;

        struct CachedModel
        {
            std::weak_ptr<const Model> Asset;
            std::size_t ImportCount = 0;
        };

        struct CachedTexture
        {
            std::weak_ptr<const Texture2D> Asset;
        };

        std::mutex g_assetMutex;
        std::unordered_map<std::string, CachedModel> g_modelCache;
        std::unordered_map<std::string, CachedTexture> g_textureCache;

        std::string ResolveAssetPath(const std::string& path)
        {
            std::error_code error;
            fs::path candidate(path);

            if (fs::exists(candidate, error))
            {
                return fs::weakly_canonical(candidate, error).string();
            }

#ifdef JEVAING_SOURCE_ROOT
            error.clear();
            candidate = fs::path(JEVAING_SOURCE_ROOT) / fs::path(path);

            if (fs::exists(candidate, error))
            {
                return fs::weakly_canonical(candidate, error).string();
            }
#endif

            return fs::absolute(fs::path(path), error).string();
        }

        std::string CanonicalCacheKey(const std::string& path)
        {
            std::string key = ResolveAssetPath(path);
            std::replace(key.begin(), key.end(), '\\', '/');

#ifdef _WIN32
            std::transform(
                key.begin(),
                key.end(),
                key.begin(),
                [](unsigned char character)
                {
                    return static_cast<char>(std::tolower(character));
                }
            );
#endif
            return key;
        }

        void AssignError(std::string* error, const std::string& message)
        {
            if (error)
            {
                *error = message;
            }
        }

        std::string ResolveTextureReference(
            const std::string& modelPath,
            const std::string& texturePath
        )
        {
            if (texturePath.empty())
            {
                return {};
            }

            if (texturePath[0] == '*')
            {
                return {};
            }

            fs::path candidate(texturePath);

            if (candidate.is_absolute())
            {
                return texturePath;
            }

            return (fs::path(modelPath).parent_path() / candidate).string();
        }
    }

    namespace Assets
    {
        std::shared_ptr<const Model> LoadModel(
            const std::string& path,
            std::string* error
        )
        {
            const std::string resolvedPath = ResolveAssetPath(path);
            const std::string cacheKey = CanonicalCacheKey(path);

            {
                std::lock_guard<std::mutex> lock(g_assetMutex);
                auto cache = g_modelCache.find(cacheKey);

                if (cache != g_modelCache.end())
                {
                    if (auto asset = cache->second.Asset.lock())
                    {
                        AssignError(error, {});
                        return asset;
                    }
                }
            }

            Internal::Geometry3D::ModelLoadOptions loadOptions;
            loadOptions.CenterAndNormalize = true;
            loadOptions.TargetExtent = 1.8f;

            Model model;
            std::string loadError;

            if (!Internal::Geometry3D::ModelLoader::Load(
                resolvedPath,
                model,
                loadOptions,
                loadError
            ))
            {
                const std::string message =
                    "Failed to load model:\n" +
                    path +
                    "\n" +
                    loadError;
                Internal::Logger::Error(message);
                AssignError(error, loadError);
                return {};
            }

            for (Material& material : model.Materials)
            {
                const std::string textureReference = ResolveTextureReference(
                    resolvedPath,
                    material.BaseColorTexturePath
                );

                if (textureReference.empty())
                {
                    continue;
                }

                std::string textureError;
                material.BaseColorTexture = LoadTexture2D(textureReference, &textureError);

                if (!material.BaseColorTexture)
                {
                    Internal::Logger::Error(
                        "Material texture could not be loaded and will be skipped: " +
                        textureReference +
                        " (" +
                        textureError +
                        ")"
                    );
                }
            }

            auto asset = std::make_shared<Model>(std::move(model));

            {
                std::lock_guard<std::mutex> lock(g_assetMutex);
                CachedModel& cached = g_modelCache[cacheKey];
                cached.Asset = asset;
                ++cached.ImportCount;
            }

            AssignError(error, {});
            return asset;
        }

        std::shared_ptr<const Texture2D> LoadTexture2D(
            const std::string& path,
            std::string* error
        )
        {
            const std::string resolvedPath = ResolveAssetPath(path);
            const std::string cacheKey = CanonicalCacheKey(path);

            {
                std::lock_guard<std::mutex> lock(g_assetMutex);
                auto cache = g_textureCache.find(cacheKey);

                if (cache != g_textureCache.end())
                {
                    if (auto asset = cache->second.Asset.lock())
                    {
                        AssignError(error, {});
                        return asset;
                    }
                }
            }

            Texture2D texture;
            std::string loadError;

            if (!Internal::Geometry3D::TextureLoader::Load(resolvedPath, texture, loadError))
            {
                Internal::Logger::Error(
                    "Failed to load texture:\n" +
                    path +
                    "\n" +
                    loadError
                );
                AssignError(error, loadError);
                return {};
            }

            auto asset = std::make_shared<Texture2D>(std::move(texture));

            {
                std::lock_guard<std::mutex> lock(g_assetMutex);
                g_textureCache[cacheKey].Asset = asset;
            }

            AssignError(error, {});
            return asset;
        }

        std::shared_ptr<const Texture2D> CreateCheckerTexture(
            int width,
            int height,
            int cellSize
        )
        {
            width = std::max(width, 2);
            height = std::max(height, 2);
            cellSize = std::max(cellSize, 1);

            auto texture = std::make_shared<Texture2D>();
            texture->SourcePath = "procedural://checker";
            texture->Width = width;
            texture->Height = height;
            texture->Channels = 4;
            texture->Format = PixelFormat::Rgba8;
            texture->Pixels.resize(
                static_cast<std::size_t>(width) *
                static_cast<std::size_t>(height) *
                4u
            );

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    const bool bright = ((x / cellSize) + (y / cellSize)) % 2 == 0;
                    const std::uint8_t r = bright ? 245 : 38;
                    const std::uint8_t g = bright ? 232 : 48;
                    const std::uint8_t b = bright ? 180 : 76;
                    const std::size_t offset =
                        (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                         static_cast<std::size_t>(x)) *
                        4u;

                    texture->Pixels[offset + 0] = r;
                    texture->Pixels[offset + 1] = g;
                    texture->Pixels[offset + 2] = b;
                    texture->Pixels[offset + 3] = 255;
                }
            }

            return texture;
        }

        void ClearCache()
        {
            std::lock_guard<std::mutex> lock(g_assetMutex);
            g_modelCache.clear();
            g_textureCache.clear();
        }

        std::size_t GetModelImportCountForPath(const std::string& path)
        {
            const std::string cacheKey = CanonicalCacheKey(path);

            std::lock_guard<std::mutex> lock(g_assetMutex);
            auto cache = g_modelCache.find(cacheKey);

            if (cache == g_modelCache.end())
            {
                return 0;
            }

            return cache->second.ImportCount;
        }
    }
}
