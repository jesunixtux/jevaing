#pragma once

#ifdef _WIN32

#include <cstddef>
#include <vector>

#include <d3d11.h>
#include <dxgi.h>

#include "../Renderer.h"

namespace Jevaing::Internal
{
    struct D3D11Vertex
    {
        float Position[3];
        float Color[4];
    };

    class D3D11Renderer final : public Renderer
    {
    public:
        explicit D3D11Renderer(RendererTestPattern testPattern);
        ~D3D11Renderer() override;

        bool Initialize(Window& window) override;
        bool Resize(int width, int height) override;
        void BeginFrame() override;
        void EndFrame() override;

        void Clear(const Color& color) override;

        void DrawTriangle(
            const Vec2& a,
            const Vec2& b,
            const Vec2& c,
            const Color& color
        ) override;

        void DrawQuad(
            const Vec2& center,
            const Vec2& size,
            const Color& color
        ) override;

        const char* GetName() const override;
        RendererBackend GetBackend() const override;

    private:
        bool Create2DPipeline();
        bool CreateRenderTarget(int width, int height);
        bool EnsureVertexCapacity(std::size_t requiredVertices);
        void AppendTestPattern();
        void FlushDrawCommands();
        void ReleaseResources();

    private:
        RendererTestPattern m_testPattern = RendererTestPattern::None;
        std::vector<D3D11Vertex> m_frameVertices;
        std::size_t m_vertexCapacity = 0;
        IDXGISwapChain* m_swapChain = nullptr;
        ID3D11Device* m_device = nullptr;
        ID3D11DeviceContext* m_context = nullptr;
        ID3D11RenderTargetView* m_renderTargetView = nullptr;
        ID3D11VertexShader* m_vertexShader = nullptr;
        ID3D11PixelShader* m_pixelShader = nullptr;
        ID3D11InputLayout* m_inputLayout = nullptr;
        ID3D11Buffer* m_vertexBuffer = nullptr;
        D3D11_VIEWPORT m_viewport = {};
    };
}

#endif
