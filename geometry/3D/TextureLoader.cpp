#include "TextureLoader.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Jevaing::Internal::Geometry3D
{
    namespace
    {
        bool IsSupportedTextureExtension(const std::filesystem::path& path)
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
                extension == ".png" ||
                extension == ".jpg" ||
                extension == ".jpeg";
        }
    }

    bool TextureLoader::Load(
        const std::string& path,
        Texture2D& texture,
        std::string& error
    )
    {
        texture = {};
        error.clear();

        if (!IsSupportedTextureExtension(std::filesystem::path(path)))
        {
            error =
                "Unsupported texture format for asset: " +
                path +
                " (supported: .png, .jpg, .jpeg)";
            return false;
        }

        int width = 0;
        int height = 0;
        int sourceChannels = 0;
        stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &sourceChannels, 4);

        if (!pixels)
        {
            const char* detail = stbi_failure_reason();
            error =
                "Failed to load texture: " +
                path +
                " (stb_image: " +
                (detail ? detail : "unknown error") +
                ")";
            return false;
        }

        const std::size_t byteCount =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height) *
            4u;

        texture.SourcePath = path;
        texture.Width = width;
        texture.Height = height;
        texture.Channels = 4;
        texture.Format = PixelFormat::Rgba8;
        texture.Pixels.assign(pixels, pixels + byteCount);

        stbi_image_free(pixels);
        return !texture.Empty();
    }
}
