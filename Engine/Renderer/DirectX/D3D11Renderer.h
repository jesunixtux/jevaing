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
        D3D11Renderer() = default;
        ~D3D11Renderer() override;

        bool Initialize(Window& window) override;
        void BeginFrame() override;
        void EndFrame() override;
        const char* GetName() const override;
        RendererBackend GetBackend() const override;

    private:
        void ReleaseResources();

    private:
        IDXGISwapChain* m_swapChain = nullptr;
        ID3D11Device* m_device = nullptr;
        ID3D11DeviceContext* m_context = nullptr;
        ID3D11RenderTargetView* m_renderTargetView = nullptr;
    };
}

#endif
