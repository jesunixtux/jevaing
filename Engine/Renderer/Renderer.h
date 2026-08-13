#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <Jevaing/Graphics2D.h>
#include <Jevaing/Graphics3D.h>

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

    enum class RendererTestPattern
    {
        None,
        Triangle,
        Penguin,
        Cube,
        ExternalModel,
        Texture,
        Material,
        Lighting,
        MultiModel,
        Mixed2D3D,
        Scene,
        Sprite,
        GpuMesh
    };

    struct RendererConfig
    {
        RendererBackend Backend = RendererBackend::None;
        RendererTestPattern TestPattern = RendererTestPattern::None;
        std::shared_ptr<const Model> TestModel;
        std::shared_ptr<const Model> SecondaryTestModel;
        std::shared_ptr<const Texture2D> TestTexture;
    };

    class Renderer : public Jevaing::Graphics2D, public Jevaing::Graphics3D
    {
    public:
        ~Renderer() override = default;

        virtual bool Initialize(Window& window) = 0;
        virtual bool Resize(int width, int height) = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual const char* GetName() const = 0;
        virtual RendererBackend GetBackend() const = 0;
        virtual std::size_t GetDebugMeshResourceCreateCount() const
        {
            return 0;
        }

        static std::unique_ptr<Renderer> Create(
            const RendererConfig& config,
            Window& window
        );

        static RendererBackend GetDefaultBackend();
        static bool IsBackendAvailable(RendererBackend backend);
    };

    const char* RendererBackendToString(RendererBackend backend);
    bool RendererBackendFromString(const std::string& name, RendererBackend& backend);
}
