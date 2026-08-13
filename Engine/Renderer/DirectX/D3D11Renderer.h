#pragma once

#ifdef _WIN32

#include <d3d11.h>
#include <dxgi.h>

#include "../Renderer.h"

namespace Jevaing::Internal
{
    class D3D11Renderer final : public Renderer
    {
    public:
        explicit D3D11Renderer(RendererTestPattern testPattern);
        ~D3D11Renderer() override;

        bool Initialize(Window& window) override;
        void BeginFrame() override;
        void EndFrame() override;
        const char* GetName() const override;
        RendererBackend GetBackend() const override;

    private:
        bool CreateTestPipeline(HWND hwnd);
        void ReleaseResources();

    private:
        RendererTestPattern m_testPattern = RendererTestPattern::Triangle;
        UINT m_vertexCount = 0;
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
