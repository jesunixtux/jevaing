#include "Renderer.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "../Core/Logger.h"
#include "../Core/Window.h"

#ifdef _WIN32
#include "DirectX/D3D11Renderer.h"
#endif

namespace Jevaing::Internal
{
    namespace
    {
        class NullRenderer final : public Renderer
        {
        public:
            bool Initialize(Window&) override
            {
                return true;
            }

            bool Resize(int, int) override
            {
                return true;
            }

            void BeginFrame() override
            {
            }

            void EndFrame() override
            {
            }

            void Clear(const Color&) override
            {
            }

            void DrawTriangle(
                const Vec2&,
                const Vec2&,
                const Vec2&,
                const Color&
            ) override
            {
            }

            void DrawQuad(
                const Vec2&,
                const Vec2&,
                const Color&
            ) override
            {
            }

            void SetCamera(const PerspectiveCamera&) override
            {
            }

            void DrawCube(
                const Transform&,
                const Color&
            ) override
            {
            }

            const char* GetName() const override
            {
                return "Null Renderer";
            }

            RendererBackend GetBackend() const override
            {
                return RendererBackend::None;
            }
        };
    }

    const char* RendererBackendToString(RendererBackend backend)
    {
        switch (backend)
        {
            case RendererBackend::None: return "None";
            case RendererBackend::DirectX: return "DirectX";
            case RendererBackend::Vulkan: return "Vulkan";
            case RendererBackend::Metal: return "Metal";
        }

        return "Unknown";
    }

    bool RendererBackendFromString(const std::string& name, RendererBackend& backend)
    {
        std::string normalized = name;
        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            }
        );

        if (normalized == "none" || normalized == "null")
        {
            backend = RendererBackend::None;
            return true;
        }

        if (normalized == "directx" || normalized == "dx11" || normalized == "d3d11")
        {
            backend = RendererBackend::DirectX;
            return true;
        }

        if (normalized == "vulkan")
        {
            backend = RendererBackend::Vulkan;
            return true;
        }

        if (normalized == "metal")
        {
            backend = RendererBackend::Metal;
            return true;
        }

        return false;
    }

    RendererBackend Renderer::GetDefaultBackend()
    {
#ifdef _WIN32
        return RendererBackend::DirectX;
#else
        return RendererBackend::None;
#endif
    }

    bool Renderer::IsBackendAvailable(RendererBackend backend)
    {
        switch (backend)
        {
            case RendererBackend::None:
                return true;

            case RendererBackend::DirectX:
#ifdef _WIN32
                return true;
#else
                return false;
#endif

            case RendererBackend::Vulkan:
            case RendererBackend::Metal:
                return false;
        }

        return false;
    }

    std::unique_ptr<Renderer> Renderer::Create(
        const RendererConfig& config,
        Window& window
    )
    {
        std::unique_ptr<Renderer> renderer;

        switch (config.Backend)
        {
            case RendererBackend::None:
                renderer = std::make_unique<NullRenderer>();
                break;

            case RendererBackend::DirectX:
#ifdef _WIN32
                renderer = std::make_unique<D3D11Renderer>(config);
                break;
#else
                Logger::Error("DirectX renderer is only available on Windows.");
                return nullptr;
#endif

            case RendererBackend::Vulkan:
            case RendererBackend::Metal:
                Logger::Error(
                    std::string("Renderer backend is not implemented yet: ") +
                    RendererBackendToString(config.Backend)
                );
                return nullptr;
        }

        if (!renderer->Initialize(window))
        {
            Logger::Error(
                std::string("Failed to initialize renderer: ") +
                RendererBackendToString(config.Backend)
            );
            return nullptr;
        }

        return renderer;
    }
}
