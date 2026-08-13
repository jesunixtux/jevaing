#pragma once

#include <memory>

namespace Jevaing::Internal
{
    class Window;

    enum class RendererBackend
    {
        None,
        DirectX,
        Vulkan,
        Metal
    };

    struct RendererConfig
    {
        RendererBackend Backend = RendererBackend::None;
    };

    class Renderer
    {
    public:
        virtual ~Renderer() = default;

        virtual bool Initialize(Window& window) = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual const char* GetName() const = 0;
        virtual RendererBackend GetBackend() const = 0;

        static std::unique_ptr<Renderer> Create(
            const RendererConfig& config,
            Window& window
        );
    };

    const char* RendererBackendToString(RendererBackend backend);
}
