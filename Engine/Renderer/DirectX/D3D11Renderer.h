#pragma once

#ifdef _WIN32

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <d3d11.h>
#include <dxgi.h>

#include "../Renderer.h"

namespace Jevaing::Internal
{
    struct D3D11Vertex
    {
        float Position[3];
        float Normal[3];
        float UV[2];
        float Color[4];
    };

    struct D3D11ObjectConstants
    {
        Mat4 Model = Mat4::Identity();
        Mat4 NormalMatrix = Mat4::Identity();
        Mat4 ModelViewProjection = Mat4::Identity();
        float BaseColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float LightDirection[4] = { -0.35f, -0.85f, 0.40f, 0.0f };
        float LightColor[4] = { 1.0f, 0.96f, 0.88f, 1.0f };
        float TextureState[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    struct D3D11DrawBatch
    {
        const Mesh* MeshData = nullptr;
        std::size_t MeshSignature = 0;
        D3D11ObjectConstants Constants;
        std::shared_ptr<const Texture2D> Texture;
    };

    struct D3D112DDrawBatch
    {
        std::size_t StartVertex = 0;
        std::size_t VertexCount = 0;
        D3D11ObjectConstants Constants;
        std::shared_ptr<const Texture2D> Texture;
    };

    struct D3D11MeshResource
    {
        ID3D11Buffer* VertexBuffer = nullptr;
        ID3D11Buffer* IndexBuffer = nullptr;
        std::uint32_t IndexCount = 0;
        std::size_t Signature = 0;
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

        void DrawSprite(
            const std::shared_ptr<const Texture2D>& texture,
            const Vec2& center,
            const Vec2& size,
            const Color& tint
        ) override;

        void SetCamera(const PerspectiveCamera& camera) override;

        void DrawCube(
            const Transform& transform,
            const Color& color
        ) override;

        void DrawMesh(
            const Mesh& mesh,
            const Transform& transform,
            const Material& material
        ) override;

        void SetDirectionalLight(const DirectionalLight& light) override;

        const char* GetName() const override;
        RendererBackend GetBackend() const override;
        std::size_t GetDebugMeshResourceCreateCount() const override;

    private:
        bool Create2DPipeline();
        bool CreateRenderTarget(int width, int height);
        bool CreateDepthBuffer(int width, int height);
        bool EnsureVertexCapacity(std::size_t requiredVertices);
        ID3D11ShaderResourceView* GetOrCreateTextureView(
            const std::shared_ptr<const Texture2D>& texture
        );
        void DrawModel(
            const Model& model,
            const Transform& transform,
            const Material* overrideMaterial = nullptr
        );
        void AppendTestPattern();
        void Flush2DDrawCommands();
        void Flush3DDrawCommands();
        void ReleaseResources();
        bool GetOrCreateMeshResource(
            const Mesh& mesh,
            D3D11MeshResource*& resource
        );
        static std::size_t CalculateMeshSignature(const Mesh& mesh);
        static std::string GetTextureCacheKey(const Texture2D& texture);
        static Mat4 CalculateNormalMatrix(const Transform& transform);

    private:
        RendererTestPattern m_testPattern = RendererTestPattern::None;
        std::shared_ptr<const Model> m_testModel;
        std::shared_ptr<const Model> m_secondaryTestModel;
        std::shared_ptr<const Texture2D> m_testTexture;
        DirectionalLight m_directionalLight;
        std::vector<D3D11Vertex> m_frame2DVertices;
        std::vector<D3D112DDrawBatch> m_2DDrawBatches;
        std::vector<D3D11DrawBatch> m_3DDrawBatches;
        std::unordered_map<const Mesh*, D3D11MeshResource> m_meshResources;
        std::unordered_map<std::string, ID3D11ShaderResourceView*> m_textureViews;
        std::size_t m_debugMeshResourceCreateCount = 0;
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
        ID3D11PixelShader* m_pixelShader2D = nullptr;
        ID3D11PixelShader* m_pixelShader3D = nullptr;
        ID3D11InputLayout* m_inputLayout = nullptr;
        ID3D11Buffer* m_constantBuffer = nullptr;
        ID3D11Buffer* m_vertexBuffer = nullptr;
        ID3D11RasterizerState* m_rasterizerState = nullptr;
        ID3D11SamplerState* m_samplerState = nullptr;
        D3D11_VIEWPORT m_viewport = {};
    };
}

#endif
