#include "Renderer.h"

#include <string>

#include "../Core/Logger.h"
#include "../Core/Window.h"

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

            void BeginFrame() override
            {
            }

            void EndFrame() override
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
            case RendererBackend::None:
                return "None";

            case RendererBackend::DirectX:
                return "DirectX";

            case RendererBackend::Vulkan:
                return "Vulkan";

            case RendererBackend::Metal:
                return "Metal";
        }

        return "Unknown";
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
