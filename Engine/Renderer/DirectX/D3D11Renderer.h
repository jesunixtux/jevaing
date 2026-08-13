#pragma once

#ifdef _WIN32

#include <cstddef>
#include <cstdint>
#include <memory>
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

    struct D3D11DrawBatch
    {
        std::size_t StartVertex = 0;
        std::size_t VertexCount = 0;
        Mat4 ModelViewProjection = Mat4::Identity();
    };

    class D3D11Renderer final : public Renderer
    {
    public:
        explicit D3D11Renderer(const RendererConfig& config);
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

        void SetCamera(const PerspectiveCamera& camera) override;

        void DrawCube(
            const Transform& transform,
            const Color& color
        ) override;

        const char* GetName() const override;
        RendererBackend GetBackend() const override;

    private:
        bool Create2DPipeline();
        bool CreateRenderTarget(int width, int height);
        bool CreateDepthBuffer(int width, int height);
        bool EnsureVertexCapacity(std::size_t requiredVertices);
        void DrawMesh(
            const Geometry3D::Mesh& mesh,
            const Transform& transform,
            const Color& tint
        );
        void AppendTestPattern();
        void Flush2DDrawCommands();
        void Flush3DDrawCommands();
        void ReleaseResources();

    private:
        RendererTestPattern m_testPattern = RendererTestPattern::None;
        std::shared_ptr<const Geometry3D::Mesh> m_testMesh;
        Color m_testMeshTint = { 1.0f, 1.0f, 1.0f, 1.0f };
        std::vector<D3D11Vertex> m_frame2DVertices;
        std::vector<D3D11Vertex> m_frame3DVertices;
        std::vector<D3D11DrawBatch> m_3DDrawBatches;
        std::size_t m_vertexCapacity = 0;
        std::uint64_t m_frameIndex = 0;
        PerspectiveCamera m_camera;
        IDXGISwapChain* m_swapChain = nullptr;
        ID3D11Device* m_device = nullptr;
        ID3D11DeviceContext* m_context = nullptr;
        ID3D11RenderTargetView* m_renderTargetView = nullptr;
        ID3D11Texture2D* m_depthStencilTexture = nullptr;
        ID3D11DepthStencilView* m_depthStencilView = nullptr;
        ID3D11VertexShader* m_vertexShader2D = nullptr;
        ID3D11VertexShader* m_vertexShader3D = nullptr;
        ID3D11PixelShader* m_pixelShader = nullptr;
        ID3D11InputLayout* m_inputLayout = nullptr;
        ID3D11Buffer* m_constantBuffer = nullptr;
        ID3D11Buffer* m_vertexBuffer = nullptr;
        ID3D11RasterizerState* m_rasterizerState = nullptr;
        D3D11_VIEWPORT m_viewport = {};
    };
}

#endif
