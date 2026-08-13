#ifdef _WIN32

#include "D3D11Renderer.h"

#include <d3dcompiler.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include <geometry/3D/Primitives/Cube.h>

#include "../../Core/Logger.h"
#include "../../Core/Window.h"

namespace Jevaing::Internal
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;

        constexpr Color DefaultClear = { 0.025f, 0.040f, 0.065f, 1.0f };
        constexpr Color Black = { 0.035f, 0.045f, 0.060f, 1.0f };
        constexpr Color White = { 0.92f, 0.95f, 0.98f, 1.0f };
        constexpr Color Orange = { 1.0f, 0.52f, 0.10f, 1.0f };

        constexpr char ShaderSource[] = R"(
cbuffer Jevaing3DConstants : register(b0)
{
    row_major float4x4 modelViewProjection;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VSOutput VSMain2D(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}

VSOutput VSMain3D(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), modelViewProjection);
    output.color = input.color;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    return input.color;
}
)";

        D3D11Vertex MakeVertex(float x, float y, const Color& color)
        {
            return {
                { x, y, 0.0f },
                { color.R, color.G, color.B, color.A }
            };
        }

        Color MultiplyColor(const Color& left, const Color& right)
        {
            return {
                left.R * right.R,
                left.G * right.G,
                left.B * right.B,
                left.A * right.A
            };
        }

        void AppendTriangleVertices(
            std::vector<D3D11Vertex>& vertices,
            float x0,
            float y0,
            float x1,
            float y1,
            float x2,
            float y2,
            const Color& color
        )
        {
            vertices.push_back(MakeVertex(x0, y0, color));
            vertices.push_back(MakeVertex(x1, y1, color));
            vertices.push_back(MakeVertex(x2, y2, color));
        }

        void AppendEllipse(
            std::vector<D3D11Vertex>& vertices,
            float centerX,
            float centerY,
            float radiusX,
            float radiusY,
            int segments,
            const Color& color
        )
        {
            for (int segment = 0; segment < segments; ++segment)
            {
                const float angle0 =
                    -2.0f * Pi * static_cast<float>(segment) /
                    static_cast<float>(segments);

                const float angle1 =
                    -2.0f * Pi * static_cast<float>(segment + 1) /
                    static_cast<float>(segments);

                AppendTriangleVertices(
                    vertices,
                    centerX,
                    centerY,
                    centerX + std::cos(angle0) * radiusX,
                    centerY + std::sin(angle0) * radiusY,
                    centerX + std::cos(angle1) * radiusX,
                    centerY + std::sin(angle1) * radiusY,
                    color
                );
            }
        }

        std::vector<D3D11Vertex> BuildTriangleVertices()
        {
            return {
                MakeVertex(0.0f, 0.62f, { 1.0f, 0.25f, 0.16f, 1.0f }),
                MakeVertex(0.62f, -0.58f, { 0.15f, 0.72f, 1.0f, 1.0f }),
                MakeVertex(-0.62f, -0.58f, { 0.35f, 1.0f, 0.35f, 1.0f })
            };
        }

        std::vector<D3D11Vertex> BuildPenguinVertices()
        {
            std::vector<D3D11Vertex> vertices;
            vertices.reserve(900);

            AppendEllipse(vertices, -0.19f, -0.77f, 0.18f, 0.075f, 24, Orange);
            AppendEllipse(vertices, 0.19f, -0.77f, 0.18f, 0.075f, 24, Orange);
            AppendEllipse(vertices, 0.0f, -0.08f, 0.46f, 0.70f, 40, Black);
            AppendEllipse(vertices, 0.0f, 0.46f, 0.39f, 0.36f, 36, Black);
            AppendEllipse(vertices, 0.0f, -0.16f, 0.29f, 0.46f, 36, White);
            AppendEllipse(vertices, -0.13f, 0.53f, 0.075f, 0.090f, 24, White);
            AppendEllipse(vertices, 0.13f, 0.53f, 0.075f, 0.090f, 24, White);
            AppendEllipse(vertices, -0.13f, 0.53f, 0.030f, 0.042f, 20, Black);
            AppendEllipse(vertices, 0.13f, 0.53f, 0.030f, 0.042f, 20, Black);
            AppendTriangleVertices(
                vertices,
                0.0f,
                0.30f,
                -0.105f,
                0.41f,
                0.105f,
                0.41f,
                Orange
            );

            return vertices;
        }

        bool CompileShader(
            const char* entryPoint,
            const char* target,
            ID3DBlob** shaderBlob
        )
        {
            UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
            compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

            ID3DBlob* errorBlob = nullptr;

            const HRESULT result = D3DCompile(
                ShaderSource,
                sizeof(ShaderSource) - 1,
                "Jevaing3DShader",
                nullptr,
                nullptr,
                entryPoint,
                target,
                compileFlags,
                0,
                shaderBlob,
                &errorBlob
            );

            if (FAILED(result))
            {
                if (errorBlob)
                {
                    const char* message = static_cast<const char*>(errorBlob->GetBufferPointer());
                    Logger::Error(
                        std::string("DirectX shader compilation failed: ") +
                        (message ? message : "unknown compiler error")
                    );
                    errorBlob->Release();
                }
                else
                {
                    Logger::Error("DirectX shader compilation failed without a compiler message.");
                }

                return false;
            }

            if (errorBlob)
            {
                errorBlob->Release();
            }

            return true;
        }
    }

    D3D11Renderer::D3D11Renderer(const RendererConfig& config)
        : m_testPattern(config.TestPattern),
          m_testMesh(config.TestMesh),
          m_testMeshTint(config.TestMeshTint)
    {
    }

    D3D11Renderer::~D3D11Renderer()
    {
        ReleaseResources();
    }

    bool D3D11Renderer::Initialize(Window& window)
    {
        HWND hwnd = static_cast<HWND>(window.GetNativeHandle());

        if (!hwnd)
        {
            Logger::Error("DirectX 11 renderer received an invalid native window handle.");
            return false;
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.OutputWindow = hwnd;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

        const HRESULT result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            &m_swapChain,
            &m_device,
            &featureLevel,
            &m_context
        );

        if (FAILED(result))
        {
            Logger::Error("D3D11CreateDeviceAndSwapChain failed.");
            ReleaseResources();
            return false;
        }

        if (!Create2DPipeline())
        {
            ReleaseResources();
            return false;
        }

        if (!CreateRenderTarget(window.GetWidth(), window.GetHeight()))
        {
            ReleaseResources();
            return false;
        }

        Logger::Info("DirectX 11 BIG BEAR GUMMY 2D/3D pipeline initialized.");
        return true;
    }

    bool D3D11Renderer::Create2DPipeline()
    {
        ID3DBlob* vertexShader2DBlob = nullptr;
        ID3DBlob* vertexShader3DBlob = nullptr;
        ID3DBlob* pixelShaderBlob = nullptr;

        if (!CompileShader("VSMain2D", "vs_5_0", &vertexShader2DBlob))
        {
            return false;
        }

        if (!CompileShader("VSMain3D", "vs_5_0", &vertexShader3DBlob))
        {
            vertexShader2DBlob->Release();
            return false;
        }

        if (!CompileShader("PSMain", "ps_5_0", &pixelShaderBlob))
        {
            vertexShader3DBlob->Release();
            vertexShader2DBlob->Release();
            return false;
        }

        const HRESULT vertexShader2DResult = m_device->CreateVertexShader(
            vertexShader2DBlob->GetBufferPointer(),
            vertexShader2DBlob->GetBufferSize(),
            nullptr,
            &m_vertexShader2D
        );

        const HRESULT vertexShader3DResult = m_device->CreateVertexShader(
            vertexShader3DBlob->GetBufferPointer(),
            vertexShader3DBlob->GetBufferSize(),
            nullptr,
            &m_vertexShader3D
        );

        const HRESULT pixelShaderResult = m_device->CreatePixelShader(
            pixelShaderBlob->GetBufferPointer(),
            pixelShaderBlob->GetBufferSize(),
            nullptr,
            &m_pixelShader
        );

        const D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            {
                "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                static_cast<UINT>(offsetof(D3D11Vertex, Position)),
                D3D11_INPUT_PER_VERTEX_DATA, 0
            },
            {
                "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                static_cast<UINT>(offsetof(D3D11Vertex, Color)),
                D3D11_INPUT_PER_VERTEX_DATA, 0
            }
        };

        const HRESULT inputLayoutResult = m_device->CreateInputLayout(
            inputElements,
            static_cast<UINT>(sizeof(inputElements) / sizeof(inputElements[0])),
            vertexShader2DBlob->GetBufferPointer(),
            vertexShader2DBlob->GetBufferSize(),
            &m_inputLayout
        );

        pixelShaderBlob->Release();
        vertexShader3DBlob->Release();
        vertexShader2DBlob->Release();

        if (
            FAILED(vertexShader2DResult) ||
            FAILED(vertexShader3DResult) ||
            FAILED(pixelShaderResult) ||
            FAILED(inputLayoutResult) ||
            !m_vertexShader2D ||
            !m_vertexShader3D ||
            !m_pixelShader ||
            !m_inputLayout
        )
        {
            Logger::Error("DirectX 11 failed to create the BIG BEAR GUMMY shader pipeline.");
            return false;
        }

        D3D11_BUFFER_DESC constantBufferDesc = {};
        constantBufferDesc.ByteWidth = sizeof(Mat4);
        constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        const HRESULT constantBufferResult = m_device->CreateBuffer(
            &constantBufferDesc,
            nullptr,
            &m_constantBuffer
        );

        if (FAILED(constantBufferResult) || !m_constantBuffer)
        {
            Logger::Error("DirectX 11 failed to create the 3D constant buffer.");
            return false;
        }

        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.DepthClipEnable = TRUE;

        const HRESULT rasterizerResult = m_device->CreateRasterizerState(
            &rasterizerDesc,
            &m_rasterizerState
        );

        if (FAILED(rasterizerResult) || !m_rasterizerState)
        {
            Logger::Error("DirectX 11 failed to create the rasterizer state.");
            return false;
        }

        return EnsureVertexCapacity(256);
    }

    bool D3D11Renderer::CreateRenderTarget(int width, int height)
    {
        if (!m_swapChain || !m_device || width <= 0 || height <= 0)
        {
            return false;
        }

        ID3D11Texture2D* backBuffer = nullptr;
        const HRESULT bufferResult = m_swapChain->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(&backBuffer)
        );

        if (FAILED(bufferResult) || !backBuffer)
        {
            Logger::Error("DirectX 11 failed to acquire the swap-chain back buffer.");
            return false;
        }

        const HRESULT viewResult = m_device->CreateRenderTargetView(
            backBuffer,
            nullptr,
            &m_renderTargetView
        );
        backBuffer->Release();

        if (FAILED(viewResult) || !m_renderTargetView)
        {
            Logger::Error("DirectX 11 failed to create the render target view.");
            return false;
        }

        if (!CreateDepthBuffer(width, height))
        {
            return false;
        }

        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;
        m_viewport.Width = static_cast<float>(width);
        m_viewport.Height = static_cast<float>(height);
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;

        return true;
    }

    bool D3D11Renderer::CreateDepthBuffer(int width, int height)
    {
        if (!m_device || width <= 0 || height <= 0)
        {
            return false;
        }

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = static_cast<UINT>(width);
        textureDesc.Height = static_cast<UINT>(height);
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        const HRESULT textureResult = m_device->CreateTexture2D(
            &textureDesc,
            nullptr,
            &m_depthStencilTexture
        );

        if (FAILED(textureResult) || !m_depthStencilTexture)
        {
            Logger::Error("DirectX 11 failed to create the depth buffer.");
            return false;
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
        viewDesc.Format = textureDesc.Format;
        viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

        const HRESULT viewResult = m_device->CreateDepthStencilView(
            m_depthStencilTexture,
            &viewDesc,
            &m_depthStencilView
        );

        if (FAILED(viewResult) || !m_depthStencilView)
        {
            Logger::Error("DirectX 11 failed to create the depth stencil view.");
            return false;
        }

        return true;
    }

    bool D3D11Renderer::Resize(int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            return true;
        }

        if (!m_swapChain || !m_context)
        {
            return false;
        }

        m_context->OMSetRenderTargets(0, nullptr, nullptr);

        if (m_renderTargetView)
        {
            m_renderTargetView->Release();
            m_renderTargetView = nullptr;
        }

        if (m_depthStencilView)
        {
            m_depthStencilView->Release();
            m_depthStencilView = nullptr;
        }

        if (m_depthStencilTexture)
        {
            m_depthStencilTexture->Release();
            m_depthStencilTexture = nullptr;
        }

        const HRESULT resizeResult = m_swapChain->ResizeBuffers(
            0,
            static_cast<UINT>(width),
            static_cast<UINT>(height),
            DXGI_FORMAT_UNKNOWN,
            0
        );

        if (FAILED(resizeResult))
        {
            Logger::Error("DirectX 11 failed to resize the swap chain.");
            return false;
        }

        return CreateRenderTarget(width, height);
    }

    bool D3D11Renderer::EnsureVertexCapacity(std::size_t requiredVertices)
    {
        if (requiredVertices <= m_vertexCapacity && m_vertexBuffer)
        {
            return true;
        }

        std::size_t newCapacity = std::max<std::size_t>(256, m_vertexCapacity);

        while (newCapacity < requiredVertices)
        {
            newCapacity *= 2;
        }

        if (m_vertexBuffer)
        {
            m_vertexBuffer->Release();
            m_vertexBuffer = nullptr;
        }

        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = static_cast<UINT>(newCapacity * sizeof(D3D11Vertex));
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        const HRESULT result = m_device->CreateBuffer(
            &bufferDesc,
            nullptr,
            &m_vertexBuffer
        );

        if (FAILED(result) || !m_vertexBuffer)
        {
            Logger::Error("DirectX 11 failed to allocate the dynamic vertex buffer.");
            m_vertexCapacity = 0;
            return false;
        }

        m_vertexCapacity = newCapacity;
        return true;
    }

    void D3D11Renderer::BeginFrame()
    {
        m_frame2DVertices.clear();
        m_frame3DVertices.clear();
        m_3DDrawBatches.clear();

        if (!m_context || !m_renderTargetView)
        {
            return;
        }

        m_context->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
        m_context->RSSetViewports(1, &m_viewport);
        m_context->RSSetState(m_rasterizerState);

        Clear(DefaultClear);

        if (m_depthStencilView)
        {
            m_context->ClearDepthStencilView(
                m_depthStencilView,
                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                1.0f,
                0
            );
        }

        AppendTestPattern();
    }

    void D3D11Renderer::EndFrame()
    {
        Flush3DDrawCommands();
        Flush2DDrawCommands();

        if (m_swapChain)
        {
            m_swapChain->Present(1, 0);
        }

        ++m_frameIndex;
    }

    void D3D11Renderer::Clear(const Color& color)
    {
        if (!m_context || !m_renderTargetView)
        {
            return;
        }

        const float clearColor[4] = {
            color.R,
            color.G,
            color.B,
            color.A
        };

        m_context->ClearRenderTargetView(m_renderTargetView, clearColor);
    }

    void D3D11Renderer::DrawTriangle(
        const Vec2& a,
        const Vec2& b,
        const Vec2& c,
        const Color& color
    )
    {
        m_frame2DVertices.push_back(MakeVertex(a.X, a.Y, color));
        m_frame2DVertices.push_back(MakeVertex(b.X, b.Y, color));
        m_frame2DVertices.push_back(MakeVertex(c.X, c.Y, color));
    }

    void D3D11Renderer::DrawQuad(
        const Vec2& center,
        const Vec2& size,
        const Color& color
    )
    {
        const float halfWidth = size.X * 0.5f;
        const float halfHeight = size.Y * 0.5f;

        const Vec2 topLeft = { center.X - halfWidth, center.Y + halfHeight };
        const Vec2 topRight = { center.X + halfWidth, center.Y + halfHeight };
        const Vec2 bottomRight = { center.X + halfWidth, center.Y - halfHeight };
        const Vec2 bottomLeft = { center.X - halfWidth, center.Y - halfHeight };

        DrawTriangle(topLeft, topRight, bottomRight, color);
        DrawTriangle(topLeft, bottomRight, bottomLeft, color);
    }

    void D3D11Renderer::SetCamera(const PerspectiveCamera& camera)
    {
        m_camera = camera;
    }

    void D3D11Renderer::DrawMesh(
        const Geometry3D::Mesh& mesh,
        const Transform& transform,
        const Color& tint
    )
    {
        if (mesh.Empty())
        {
            return;
        }

        D3D11DrawBatch batch;
        batch.StartVertex = m_frame3DVertices.size();

        for (const Geometry3D::MeshVertex& vertex : mesh.Vertices)
        {
            const Color color = MultiplyColor(vertex.VertexColor, tint);

            m_frame3DVertices.push_back({
                {
                    vertex.Position.X,
                    vertex.Position.Y,
                    vertex.Position.Z
                },
                {
                    color.R,
                    color.G,
                    color.B,
                    color.A
                }
            });
        }

        batch.VertexCount = m_frame3DVertices.size() - batch.StartVertex;
        batch.ModelViewProjection =
            transform.ToMatrix() *
            m_camera.GetViewMatrix() *
            m_camera.GetProjectionMatrix();

        m_3DDrawBatches.push_back(batch);
    }

    void D3D11Renderer::DrawCube(
        const Transform& transform,
        const Color& color
    )
    {
        static const Geometry3D::Mesh cube = Geometry3D::CreateCubeMesh();
        DrawMesh(cube, transform, color);
    }

    void D3D11Renderer::AppendTestPattern()
    {
        std::vector<D3D11Vertex> vertices;

        switch (m_testPattern)
        {
            case RendererTestPattern::None:
                return;

            case RendererTestPattern::Triangle:
                vertices = BuildTriangleVertices();
                break;

            case RendererTestPattern::Penguin:
                vertices = BuildPenguinVertices();
                break;

            case RendererTestPattern::Cube:
            {
                PerspectiveCamera camera;
                camera.Position = { 0.0f, 1.25f, -4.5f };
                camera.Target = { 0.0f, 0.0f, 0.0f };
                camera.AspectRatio =
                    m_viewport.Height > 0.0f
                        ? m_viewport.Width / m_viewport.Height
                        : 1.0f;
                SetCamera(camera);

                Transform transform;
                transform.Rotation = {
                    static_cast<float>(m_frameIndex) * 0.010f,
                    static_cast<float>(m_frameIndex) * 0.017f,
                    0.0f
                };
                DrawCube(transform, { 0.25f, 0.72f, 1.0f, 1.0f });
                return;
            }

            case RendererTestPattern::ExternalModel:
            {
                if (!m_testMesh || m_testMesh->Empty())
                {
                    return;
                }

                PerspectiveCamera camera;
                camera.Position = { 0.0f, 0.45f, -4.5f };
                camera.Target = { 0.0f, 0.0f, 0.0f };
                camera.AspectRatio =
                    m_viewport.Height > 0.0f
                        ? m_viewport.Width / m_viewport.Height
                        : 1.0f;
                SetCamera(camera);

                Transform transform;
                transform.Rotation = {
                    static_cast<float>(m_frameIndex) * 0.008f,
                    static_cast<float>(m_frameIndex) * 0.016f,
                    0.0f
                };

                DrawMesh(*m_testMesh, transform, m_testMeshTint);
                return;
            }
        }

        m_frame2DVertices.insert(
            m_frame2DVertices.end(),
            vertices.begin(),
            vertices.end()
        );
    }

    void D3D11Renderer::Flush2DDrawCommands()
    {
        if (m_frame2DVertices.empty() || !m_context)
        {
            return;
        }

        if (!EnsureVertexCapacity(m_frame2DVertices.size()))
        {
            return;
        }

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        const HRESULT mapResult = m_context->Map(
            m_vertexBuffer,
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mapped
        );

        if (FAILED(mapResult) || !mapped.pData)
        {
            Logger::Error("DirectX 11 failed to map the dynamic 2D vertex buffer.");
            return;
        }

        std::memcpy(
            mapped.pData,
            m_frame2DVertices.data(),
            m_frame2DVertices.size() * sizeof(D3D11Vertex)
        );
        m_context->Unmap(m_vertexBuffer, 0);

        constexpr UINT stride = sizeof(D3D11Vertex);
        constexpr UINT offset = 0;

        m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
        m_context->IASetInputLayout(m_inputLayout);
        m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->VSSetShader(m_vertexShader2D, nullptr, 0);
        m_context->PSSetShader(m_pixelShader, nullptr, 0);
        m_context->Draw(static_cast<UINT>(m_frame2DVertices.size()), 0);
    }

    void D3D11Renderer::Flush3DDrawCommands()
    {
        if (m_frame3DVertices.empty() || m_3DDrawBatches.empty() || !m_context)
        {
            return;
        }

        if (!EnsureVertexCapacity(m_frame3DVertices.size()))
        {
            return;
        }

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        const HRESULT mapResult = m_context->Map(
            m_vertexBuffer,
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mapped
        );

        if (FAILED(mapResult) || !mapped.pData)
        {
            Logger::Error("DirectX 11 failed to map the dynamic 3D vertex buffer.");
            return;
        }

        std::memcpy(
            mapped.pData,
            m_frame3DVertices.data(),
            m_frame3DVertices.size() * sizeof(D3D11Vertex)
        );
        m_context->Unmap(m_vertexBuffer, 0);

        constexpr UINT stride = sizeof(D3D11Vertex);
        constexpr UINT offset = 0;

        m_context->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
        m_context->IASetInputLayout(m_inputLayout);
        m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->VSSetShader(m_vertexShader3D, nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
        m_context->PSSetShader(m_pixelShader, nullptr, 0);

        for (const D3D11DrawBatch& batch : m_3DDrawBatches)
        {
            m_context->UpdateSubresource(
                m_constantBuffer,
                0,
                nullptr,
                &batch.ModelViewProjection,
                0,
                0
            );

            m_context->Draw(
                static_cast<UINT>(batch.VertexCount),
                static_cast<UINT>(batch.StartVertex)
            );
        }
    }

    const char* D3D11Renderer::GetName() const
    {
        return "DirectX 11 Renderer";
    }

    RendererBackend D3D11Renderer::GetBackend() const
    {
        return RendererBackend::DirectX;
    }

    void D3D11Renderer::ReleaseResources()
    {
        if (m_context)
        {
            m_context->ClearState();
        }

        if (m_vertexBuffer)
        {
            m_vertexBuffer->Release();
            m_vertexBuffer = nullptr;
        }

        if (m_constantBuffer)
        {
            m_constantBuffer->Release();
            m_constantBuffer = nullptr;
        }

        if (m_rasterizerState)
        {
            m_rasterizerState->Release();
            m_rasterizerState = nullptr;
        }

        if (m_inputLayout)
        {
            m_inputLayout->Release();
            m_inputLayout = nullptr;
        }

        if (m_pixelShader)
        {
            m_pixelShader->Release();
            m_pixelShader = nullptr;
        }

        if (m_vertexShader3D)
        {
            m_vertexShader3D->Release();
            m_vertexShader3D = nullptr;
        }

        if (m_vertexShader2D)
        {
            m_vertexShader2D->Release();
            m_vertexShader2D = nullptr;
        }

        if (m_depthStencilView)
        {
            m_depthStencilView->Release();
            m_depthStencilView = nullptr;
        }

        if (m_depthStencilTexture)
        {
            m_depthStencilTexture->Release();
            m_depthStencilTexture = nullptr;
        }

        if (m_renderTargetView)
        {
            m_renderTargetView->Release();
            m_renderTargetView = nullptr;
        }

        if (m_swapChain)
        {
            m_swapChain->Release();
            m_swapChain = nullptr;
        }

        if (m_context)
        {
            m_context->Release();
            m_context = nullptr;
        }

        if (m_device)
        {
            m_device->Release();
            m_device = nullptr;
        }

        m_vertexCapacity = 0;
        m_frame2DVertices.clear();
        m_frame3DVertices.clear();
        m_3DDrawBatches.clear();
    }
}

#endif
