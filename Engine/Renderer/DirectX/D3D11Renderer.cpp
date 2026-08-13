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
    row_major float4x4 model;
    row_major float4x4 normalMatrix;
    row_major float4x4 modelViewProjection;
    float4 baseColor;
    float4 lightDirection;
    float4 lightColor;
    float4 textureState;
};

Texture2D baseColorTexture : register(t0);
SamplerState baseColorSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

VSOutput VSMain2D(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0f);
    output.normal = input.normal;
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

VSOutput VSMain3D(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), modelViewProjection);
    output.normal = normalize(mul(float4(input.normal, 0.0f), normalMatrix).xyz);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 PSMain2D(VSOutput input) : SV_TARGET
{
    float4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

    if (textureState.x > 0.5f)
    {
        textureColor = baseColorTexture.Sample(baseColorSampler, input.uv);
    }

    return input.color * textureColor;
}

float4 PSMain3D(VSOutput input) : SV_TARGET
{
    float4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

    if (textureState.x > 0.5f)
    {
        textureColor = baseColorTexture.Sample(baseColorSampler, input.uv);
    }

    float3 normal = normalize(input.normal);
    float3 lightToSurface = normalize(-lightDirection.xyz);
    float diffuse = saturate(dot(normal, lightToSurface)) * lightDirection.w;
    float3 lighting = 0.18f + diffuse * lightColor.rgb;

    float4 materialColor = input.color * baseColor * textureColor;
    return float4(materialColor.rgb * lighting, materialColor.a);
}
)";

        D3D11Vertex MakeVertex(float x, float y, const Color& color)
        {
            return {
                { x, y, 0.0f },
                { 0.0f, 0.0f, -1.0f },
                { 0.0f, 0.0f },
                { color.R, color.G, color.B, color.A }
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

        void Append2DQuadVertices(
            std::vector<D3D11Vertex>& vertices,
            const Vec2& center,
            const Vec2& size,
            const Color& color
        )
        {
            const float halfWidth = size.X * 0.5f;
            const float halfHeight = size.Y * 0.5f;

            const float left = center.X - halfWidth;
            const float right = center.X + halfWidth;
            const float top = center.Y + halfHeight;
            const float bottom = center.Y - halfHeight;

            vertices.push_back({ { left, top, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, { color.R, color.G, color.B, color.A } });
            vertices.push_back({ { right, top, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f }, { color.R, color.G, color.B, color.A } });
            vertices.push_back({ { right, bottom, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }, { color.R, color.G, color.B, color.A } });
            vertices.push_back({ { left, top, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, { color.R, color.G, color.B, color.A } });
            vertices.push_back({ { right, bottom, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }, { color.R, color.G, color.B, color.A } });
            vertices.push_back({ { left, bottom, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }, { color.R, color.G, color.B, color.A } });
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
          m_testModel(config.TestModel),
          m_secondaryTestModel(config.SecondaryTestModel),
          m_testTexture(config.TestTexture)
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
        ID3DBlob* pixelShader2DBlob = nullptr;
        ID3DBlob* pixelShader3DBlob = nullptr;

        if (!CompileShader("VSMain2D", "vs_5_0", &vertexShader2DBlob))
        {
            return false;
        }

        if (!CompileShader("VSMain3D", "vs_5_0", &vertexShader3DBlob))
        {
            vertexShader2DBlob->Release();
            return false;
        }

        if (!CompileShader("PSMain2D", "ps_5_0", &pixelShader2DBlob))
        {
            vertexShader3DBlob->Release();
            vertexShader2DBlob->Release();
            return false;
        }

        if (!CompileShader("PSMain3D", "ps_5_0", &pixelShader3DBlob))
        {
            pixelShader2DBlob->Release();
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

        const HRESULT pixelShader2DResult = m_device->CreatePixelShader(
            pixelShader2DBlob->GetBufferPointer(),
            pixelShader2DBlob->GetBufferSize(),
            nullptr,
            &m_pixelShader2D
        );

        const HRESULT pixelShader3DResult = m_device->CreatePixelShader(
            pixelShader3DBlob->GetBufferPointer(),
            pixelShader3DBlob->GetBufferSize(),
            nullptr,
            &m_pixelShader3D
        );

        const D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            {
                "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                static_cast<UINT>(offsetof(D3D11Vertex, Position)),
                D3D11_INPUT_PER_VERTEX_DATA, 0
            },
            {
                "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
                static_cast<UINT>(offsetof(D3D11Vertex, Normal)),
                D3D11_INPUT_PER_VERTEX_DATA, 0
            },
            {
                "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                static_cast<UINT>(offsetof(D3D11Vertex, UV)),
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

        pixelShader3DBlob->Release();
        pixelShader2DBlob->Release();
        vertexShader3DBlob->Release();
        vertexShader2DBlob->Release();

        if (
            FAILED(vertexShader2DResult) ||
            FAILED(vertexShader3DResult) ||
            FAILED(pixelShader2DResult) ||
            FAILED(pixelShader3DResult) ||
            FAILED(inputLayoutResult) ||
            !m_vertexShader2D ||
            !m_vertexShader3D ||
            !m_pixelShader2D ||
            !m_pixelShader3D ||
            !m_inputLayout
        )
        {
            Logger::Error("DirectX 11 failed to create the BIG BEAR GUMMY shader pipeline.");
            return false;
        }

        D3D11_BUFFER_DESC constantBufferDesc = {};
        constantBufferDesc.ByteWidth = sizeof(D3D11ObjectConstants);
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

        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

        const HRESULT samplerResult = m_device->CreateSamplerState(
            &samplerDesc,
            &m_samplerState
        );

        if (FAILED(samplerResult) || !m_samplerState)
        {
            Logger::Error("DirectX 11 failed to create the texture sampler state.");
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
        m_2DDrawBatches.clear();
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

        D3D112DDrawBatch batch;
        batch.StartVertex = m_frame2DVertices.size() - 3;
        batch.VertexCount = 3;
        m_2DDrawBatches.push_back(batch);
    }

    void D3D11Renderer::DrawQuad(
        const Vec2& center,
        const Vec2& size,
        const Color& color
    )
    {
        D3D112DDrawBatch batch;
        batch.StartVertex = m_frame2DVertices.size();
        Append2DQuadVertices(m_frame2DVertices, center, size, color);
        batch.VertexCount = m_frame2DVertices.size() - batch.StartVertex;
        m_2DDrawBatches.push_back(batch);
    }

    void D3D11Renderer::DrawSprite(
        const std::shared_ptr<const Texture2D>& texture,
        const Vec2& center,
        const Vec2& size,
        const Color& tint
    )
    {
        D3D112DDrawBatch batch;
        batch.StartVertex = m_frame2DVertices.size();
        Append2DQuadVertices(m_frame2DVertices, center, size, tint);
        batch.VertexCount = m_frame2DVertices.size() - batch.StartVertex;
        batch.Texture = texture;
        batch.Constants.TextureState[0] = texture && !texture->Empty() ? 1.0f : 0.0f;
        m_2DDrawBatches.push_back(batch);
    }

    void D3D11Renderer::SetCamera(const PerspectiveCamera& camera)
    {
        m_camera = camera;
    }

    ID3D11ShaderResourceView* D3D11Renderer::GetOrCreateTextureView(
        const std::shared_ptr<const Texture2D>& texture
    )
    {
        if (!texture || texture->Empty() || texture->Format != PixelFormat::Rgba8)
        {
            return nullptr;
        }

        const std::string cacheKey = GetTextureCacheKey(*texture);
        auto existing = m_textureViews.find(cacheKey);

        if (existing != m_textureViews.end())
        {
            return existing->second;
        }

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = static_cast<UINT>(texture->Width);
        textureDesc.Height = static_cast<UINT>(texture->Height);
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA textureData = {};
        textureData.pSysMem = texture->Pixels.data();
        textureData.SysMemPitch = static_cast<UINT>(texture->Width * 4);

        ID3D11Texture2D* gpuTexture = nullptr;
        const HRESULT textureResult = m_device->CreateTexture2D(
            &textureDesc,
            &textureData,
            &gpuTexture
        );

        if (FAILED(textureResult) || !gpuTexture)
        {
            Logger::Error("DirectX 11 failed to create a Texture2D GPU resource.");
            return nullptr;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
        viewDesc.Format = textureDesc.Format;
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipLevels = 1;

        ID3D11ShaderResourceView* shaderResourceView = nullptr;
        const HRESULT viewResult = m_device->CreateShaderResourceView(
            gpuTexture,
            &viewDesc,
            &shaderResourceView
        );
        gpuTexture->Release();

        if (FAILED(viewResult) || !shaderResourceView)
        {
            Logger::Error("DirectX 11 failed to create a Texture2D shader view.");
            return nullptr;
        }

        m_textureViews[cacheKey] = shaderResourceView;
        return shaderResourceView;
    }

    std::size_t D3D11Renderer::CalculateMeshSignature(const Mesh& mesh)
    {
        std::size_t signature = mesh.Vertices.size() * 1469598103934665603ull;
        signature ^= mesh.Indices.size() + 0x9e3779b97f4a7c15ull + (signature << 6) + (signature >> 2);
        signature ^= mesh.Name.size() + 0x9e3779b97f4a7c15ull + (signature << 6) + (signature >> 2);

        if (!mesh.Vertices.empty())
        {
            signature ^= static_cast<std::size_t>(
                std::fabs(mesh.Vertices.front().Position.X) * 100000.0f
            );
            signature ^= static_cast<std::size_t>(
                std::fabs(mesh.Vertices.back().Position.Z) * 100000.0f
            ) << 1;
        }

        return signature;
    }

    std::string D3D11Renderer::GetTextureCacheKey(const Texture2D& texture)
    {
        return
            texture.SourcePath +
            "|" +
            std::to_string(texture.Width) +
            "x" +
            std::to_string(texture.Height) +
            "|" +
            std::to_string(static_cast<int>(texture.Format));
    }

    Mat4 D3D11Renderer::CalculateNormalMatrix(const Transform& transform)
    {
        Transform normalTransform = transform;

        normalTransform.Scale = {
            std::fabs(transform.Scale.X) > 0.000001f ? 1.0f / transform.Scale.X : 1.0f,
            std::fabs(transform.Scale.Y) > 0.000001f ? 1.0f / transform.Scale.Y : 1.0f,
            std::fabs(transform.Scale.Z) > 0.000001f ? 1.0f / transform.Scale.Z : 1.0f
        };

        return
            Mat4::Scale(normalTransform.Scale) *
            Mat4::RotationX(normalTransform.Rotation.X) *
            Mat4::RotationY(normalTransform.Rotation.Y) *
            Mat4::RotationZ(normalTransform.Rotation.Z);
    }

    bool D3D11Renderer::GetOrCreateMeshResource(
        const Mesh& mesh,
        D3D11MeshResource*& resource
    )
    {
        resource = nullptr;

        if (mesh.Empty() || !m_device)
        {
            return false;
        }

        const std::size_t signature = CalculateMeshSignature(mesh);
        D3D11MeshResource& candidate = m_meshResources[&mesh];

        if (
            candidate.VertexBuffer &&
            candidate.IndexBuffer &&
            candidate.Signature == signature
        )
        {
            resource = &candidate;
            return true;
        }

        if (candidate.VertexBuffer)
        {
            candidate.VertexBuffer->Release();
            candidate.VertexBuffer = nullptr;
        }

        if (candidate.IndexBuffer)
        {
            candidate.IndexBuffer->Release();
            candidate.IndexBuffer = nullptr;
        }

        std::vector<D3D11Vertex> vertices;
        vertices.reserve(mesh.Vertices.size());

        for (const Vertex3D& vertex : mesh.Vertices)
        {
            vertices.push_back({
                { vertex.Position.X, vertex.Position.Y, vertex.Position.Z },
                { vertex.Normal.X, vertex.Normal.Y, vertex.Normal.Z },
                { vertex.UV.X, vertex.UV.Y },
                {
                    vertex.VertexColor.R,
                    vertex.VertexColor.G,
                    vertex.VertexColor.B,
                    vertex.VertexColor.A
                }
            });
        }

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(D3D11Vertex));
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = vertices.data();

        const HRESULT vertexResult = m_device->CreateBuffer(
            &vertexBufferDesc,
            &vertexData,
            &candidate.VertexBuffer
        );

        if (FAILED(vertexResult) || !candidate.VertexBuffer)
        {
            Logger::Error("DirectX 11 failed to create a persistent mesh vertex buffer.");
            return false;
        }

        D3D11_BUFFER_DESC indexBufferDesc = {};
        indexBufferDesc.ByteWidth =
            static_cast<UINT>(mesh.Indices.size() * sizeof(std::uint32_t));
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexData = {};
        indexData.pSysMem = mesh.Indices.data();

        const HRESULT indexResult = m_device->CreateBuffer(
            &indexBufferDesc,
            &indexData,
            &candidate.IndexBuffer
        );

        if (FAILED(indexResult) || !candidate.IndexBuffer)
        {
            Logger::Error("DirectX 11 failed to create a persistent mesh index buffer.");
            candidate.VertexBuffer->Release();
            candidate.VertexBuffer = nullptr;
            return false;
        }

        candidate.IndexCount = static_cast<std::uint32_t>(mesh.Indices.size());
        candidate.Signature = signature;
        ++m_debugMeshResourceCreateCount;
        resource = &candidate;
        return true;
    }

    void D3D11Renderer::DrawMesh(
        const Mesh& mesh,
        const Transform& transform,
        const Material& material
    )
    {
        if (mesh.Empty())
        {
            return;
        }

        D3D11DrawBatch batch;
        batch.MeshData = &mesh;
        batch.MeshSignature = CalculateMeshSignature(mesh);
        batch.Constants.Model = transform.ToMatrix();
        batch.Constants.NormalMatrix = CalculateNormalMatrix(transform);
        batch.Constants.ModelViewProjection =
            batch.Constants.Model *
            m_camera.GetViewMatrix() *
            m_camera.GetProjectionMatrix();
        batch.Constants.BaseColor[0] = material.BaseColor.R;
        batch.Constants.BaseColor[1] = material.BaseColor.G;
        batch.Constants.BaseColor[2] = material.BaseColor.B;
        batch.Constants.BaseColor[3] = material.BaseColor.A;
        batch.Constants.LightDirection[0] = m_directionalLight.Direction.X;
        batch.Constants.LightDirection[1] = m_directionalLight.Direction.Y;
        batch.Constants.LightDirection[2] = m_directionalLight.Direction.Z;
        batch.Constants.LightDirection[3] = m_directionalLight.Intensity;
        batch.Constants.LightColor[0] = m_directionalLight.Color.R;
        batch.Constants.LightColor[1] = m_directionalLight.Color.G;
        batch.Constants.LightColor[2] = m_directionalLight.Color.B;
        batch.Constants.LightColor[3] = m_directionalLight.Color.A;
        batch.Constants.TextureState[0] =
            material.BaseColorTexture && !material.BaseColorTexture->Empty()
                ? 1.0f
                : 0.0f;
        batch.Texture = material.BaseColorTexture;

        m_3DDrawBatches.push_back(batch);
    }

    void D3D11Renderer::DrawCube(
        const Transform& transform,
        const Color& color
    )
    {
        static const Mesh cube = Geometry3D::CreateCubeMesh();

        Material material;
        material.BaseColor = color;
        DrawMesh(cube, transform, material);
    }

    void D3D11Renderer::SetDirectionalLight(const DirectionalLight& light)
    {
        m_directionalLight = light;
    }

    void D3D11Renderer::DrawModel(
        const Model& model,
        const Transform& transform,
        const Material* overrideMaterial
    )
    {
        for (const Mesh& mesh : model.Meshes)
        {
            const Material fallback;
            const Material& material =
                overrideMaterial
                    ? *overrideMaterial
                    : (
                        mesh.MaterialIndex < model.Materials.size()
                            ? model.Materials[mesh.MaterialIndex]
                            : fallback
                    );

            DrawMesh(mesh, transform, material);
        }
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
                if (!m_testModel || m_testModel->Empty())
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

                DrawModel(*m_testModel, transform);
                return;
            }

            case RendererTestPattern::Texture:
            {
                PerspectiveCamera camera;
                camera.Position = { 0.0f, 1.10f, -3.6f };
                camera.Target = { 0.0f, 0.0f, 0.0f };
                camera.AspectRatio =
                    m_viewport.Height > 0.0f
                        ? m_viewport.Width / m_viewport.Height
                        : 1.0f;
                SetCamera(camera);

                static const Mesh plane = Geometry3D::CreatePlaneMesh();
                Material material;
                material.BaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
                material.BaseColorTexture = m_testTexture;

                Transform transform;
                transform.Position = { 0.0f, 0.0f, 0.0f };
                transform.Rotation = { -0.55f, 0.0f, 0.0f };
                transform.Scale = { 2.6f, 2.6f, 2.6f };

                DrawMesh(plane, transform, material);
                return;
            }

            case RendererTestPattern::Material:
            {
                PerspectiveCamera camera;
                camera.Position = { 0.0f, 1.00f, -5.2f };
                camera.Target = { 0.0f, 0.0f, 0.0f };
                camera.AspectRatio =
                    m_viewport.Height > 0.0f
                        ? m_viewport.Width / m_viewport.Height
                        : 1.0f;
                SetCamera(camera);

                static const Mesh cube = Geometry3D::CreateCubeMesh();
                const Color colors[] = {
                    { 1.0f, 0.18f, 0.16f, 1.0f },
                    { 0.18f, 0.86f, 0.36f, 1.0f },
                    { 0.18f, 0.42f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f }
                };

                for (int item = 0; item < 4; ++item)
                {
                    Material material;
                    material.BaseColor = colors[item];

                    if (item == 3)
                    {
                        material.BaseColorTexture = m_testTexture;
                    }

                    Transform transform;
                    transform.Position = { -2.1f + static_cast<float>(item) * 1.4f, 0.0f, 0.0f };
                    transform.Rotation = {
                        static_cast<float>(m_frameIndex) * 0.010f,
                        static_cast<float>(m_frameIndex) * 0.015f,
                        0.0f
                    };
                    DrawMesh(cube, transform, material);
                }

                return;
            }

            case RendererTestPattern::Lighting:
            {
                PerspectiveCamera camera;
                camera.Position = { 0.0f, 1.15f, -4.2f };
                camera.Target = { 0.0f, 0.0f, 0.0f };
                camera.AspectRatio =
                    m_viewport.Height > 0.0f
                        ? m_viewport.Width / m_viewport.Height
                        : 1.0f;
                SetCamera(camera);

                const float angle = static_cast<float>(m_frameIndex) * 0.018f;
                DirectionalLight light;
                light.Direction = { std::cos(angle) * -0.75f, -0.85f, std::sin(angle) * 0.75f };
                light.Color = { 1.0f, 0.95f, 0.84f, 1.0f };
                light.Intensity = 1.15f;
                SetDirectionalLight(light);

                static const Mesh cube = Geometry3D::CreateCubeMesh();
                Material material;
                material.BaseColor = { 0.38f, 0.76f, 1.0f, 1.0f };

                Transform transform;
                transform.Rotation = {
                    static_cast<float>(m_frameIndex) * 0.006f,
                    static_cast<float>(m_frameIndex) * 0.013f,
                    0.0f
                };
                transform.Scale = { 1.55f, 1.55f, 1.55f };
                DrawMesh(cube, transform, material);
                return;
            }

            case RendererTestPattern::MultiModel:
            {
                PerspectiveCamera camera;
                camera.Position = { 0.0f, 1.20f, -6.4f };
                camera.Target = { 0.0f, 0.0f, 0.0f };
                camera.AspectRatio =
                    m_viewport.Height > 0.0f
                        ? m_viewport.Width / m_viewport.Height
                        : 1.0f;
                SetCamera(camera);

                const float rotation = static_cast<float>(m_frameIndex) * 0.010f;

                if (m_testModel)
                {
                    Transform transform;
                    transform.Position = { -2.3f, 0.0f, 0.0f };
                    transform.Rotation = { 0.0f, rotation, 0.0f };
                    DrawModel(*m_testModel, transform);
                }

                if (m_secondaryTestModel)
                {
                    Transform transform;
                    transform.Position = { 0.0f, 0.0f, 0.0f };
                    transform.Rotation = { 0.0f, -rotation * 0.85f, 0.0f };
                    DrawModel(*m_secondaryTestModel, transform);
                }

                static const Mesh cube = Geometry3D::CreateCubeMesh();
                Material cubeMaterial;
                cubeMaterial.BaseColor = { 0.25f, 0.72f, 1.0f, 1.0f };

                Transform cubeTransform;
                cubeTransform.Position = { 2.3f, 0.0f, 0.0f };
                cubeTransform.Rotation = { rotation * 0.6f, rotation, 0.0f };
                cubeTransform.Scale = { 1.25f, 1.25f, 1.25f };
                DrawMesh(cube, cubeTransform, cubeMaterial);
                return;
            }

            case RendererTestPattern::Mixed2D3D:
            {
                PerspectiveCamera camera;
                camera.Position = { 0.0f, 1.10f, -4.0f };
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

                DrawQuad(
                    { 0.0f, -0.88f },
                    { 1.65f, 0.16f },
                    { 0.08f, 0.12f, 0.18f, 1.0f }
                );
                DrawQuad(
                    { -0.72f, -0.88f },
                    { 0.18f, 0.06f },
                    { 0.95f, 0.36f, 0.22f, 1.0f }
                );
                return;
            }

            case RendererTestPattern::Scene:
                return;

            case RendererTestPattern::Sprite:
            {
                DrawSprite(
                    m_testTexture,
                    { 0.0f, 0.0f },
                    { 0.65f, 0.65f },
                    { 1.0f, 1.0f, 1.0f, 1.0f }
                );
                return;
            }

            case RendererTestPattern::GpuMesh:
            {
                PerspectiveCamera camera;
                camera.Position = { 0.0f, 2.3f, -8.0f };
                camera.Target = { 0.0f, 0.0f, 0.0f };
                camera.AspectRatio =
                    m_viewport.Height > 0.0f
                        ? m_viewport.Width / m_viewport.Height
                        : 1.0f;
                SetCamera(camera);

                static const Mesh cube = Geometry3D::CreateCubeMesh();
                Material material;
                material.BaseColor = { 0.34f, 0.76f, 1.0f, 1.0f };

                for (int z = 0; z < 4; ++z)
                {
                    for (int x = 0; x < 6; ++x)
                    {
                        Transform transform;
                        transform.Position = {
                            -3.0f + static_cast<float>(x) * 1.2f,
                            0.0f,
                            static_cast<float>(z) * 1.0f
                        };
                        transform.Rotation = {
                            0.0f,
                            static_cast<float>(m_frameIndex) * 0.01f,
                            0.0f
                        };
                        DrawMesh(cube, transform, material);
                    }
                }

                return;
            }
        }

        const std::size_t startVertex = m_frame2DVertices.size();
        m_frame2DVertices.insert(
            m_frame2DVertices.end(),
            vertices.begin(),
            vertices.end()
        );

        if (!vertices.empty())
        {
            D3D112DDrawBatch batch;
            batch.StartVertex = startVertex;
            batch.VertexCount = vertices.size();
            m_2DDrawBatches.push_back(batch);
        }
    }

    void D3D11Renderer::Flush2DDrawCommands()
    {
        if (m_frame2DVertices.empty() || m_2DDrawBatches.empty() || !m_context)
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
        m_context->PSSetShader(m_pixelShader2D, nullptr, 0);
        m_context->PSSetConstantBuffers(0, 1, &m_constantBuffer);
        m_context->PSSetSamplers(0, 1, &m_samplerState);

        for (const D3D112DDrawBatch& batch : m_2DDrawBatches)
        {
            ID3D11ShaderResourceView* textureView = GetOrCreateTextureView(batch.Texture);
            m_context->PSSetShaderResources(0, 1, &textureView);
            m_context->UpdateSubresource(
                m_constantBuffer,
                0,
                nullptr,
                &batch.Constants,
                0,
                0
            );
            m_context->Draw(
                static_cast<UINT>(batch.VertexCount),
                static_cast<UINT>(batch.StartVertex)
            );
        }

        ID3D11ShaderResourceView* nullTexture = nullptr;
        m_context->PSSetShaderResources(0, 1, &nullTexture);
    }

    void D3D11Renderer::Flush3DDrawCommands()
    {
        if (m_3DDrawBatches.empty() || !m_context)
        {
            return;
        }

        constexpr UINT stride = sizeof(D3D11Vertex);
        constexpr UINT offset = 0;

        m_context->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
        m_context->IASetInputLayout(m_inputLayout);
        m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->VSSetShader(m_vertexShader3D, nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
        m_context->PSSetShader(m_pixelShader3D, nullptr, 0);
        m_context->PSSetConstantBuffers(0, 1, &m_constantBuffer);
        m_context->PSSetSamplers(0, 1, &m_samplerState);

        for (const D3D11DrawBatch& batch : m_3DDrawBatches)
        {
            if (!batch.MeshData)
            {
                continue;
            }

            D3D11MeshResource* meshResource = nullptr;

            if (!GetOrCreateMeshResource(*batch.MeshData, meshResource) || !meshResource)
            {
                continue;
            }

            ID3D11ShaderResourceView* textureView = GetOrCreateTextureView(batch.Texture);
            m_context->PSSetShaderResources(0, 1, &textureView);
            m_context->IASetVertexBuffers(
                0,
                1,
                &meshResource->VertexBuffer,
                &stride,
                &offset
            );
            m_context->IASetIndexBuffer(
                meshResource->IndexBuffer,
                DXGI_FORMAT_R32_UINT,
                0
            );

            m_context->UpdateSubresource(
                m_constantBuffer,
                0,
                nullptr,
                &batch.Constants,
                0,
                0
            );

            m_context->DrawIndexed(
                meshResource->IndexCount,
                0,
                0
            );
        }

        ID3D11ShaderResourceView* nullTexture = nullptr;
        m_context->PSSetShaderResources(0, 1, &nullTexture);
    }

    const char* D3D11Renderer::GetName() const
    {
        return "DirectX 11 Renderer";
    }

    RendererBackend D3D11Renderer::GetBackend() const
    {
        return RendererBackend::DirectX;
    }

    std::size_t D3D11Renderer::GetDebugMeshResourceCreateCount() const
    {
        return m_debugMeshResourceCreateCount;
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

        for (auto& textureView : m_textureViews)
        {
            if (textureView.second)
            {
                textureView.second->Release();
            }
        }
        m_textureViews.clear();

        for (auto& meshResource : m_meshResources)
        {
            if (meshResource.second.VertexBuffer)
            {
                meshResource.second.VertexBuffer->Release();
            }

            if (meshResource.second.IndexBuffer)
            {
                meshResource.second.IndexBuffer->Release();
            }
        }
        m_meshResources.clear();

        if (m_constantBuffer)
        {
            m_constantBuffer->Release();
            m_constantBuffer = nullptr;
        }

        if (m_samplerState)
        {
            m_samplerState->Release();
            m_samplerState = nullptr;
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

        if (m_pixelShader3D)
        {
            m_pixelShader3D->Release();
            m_pixelShader3D = nullptr;
        }

        if (m_pixelShader2D)
        {
            m_pixelShader2D->Release();
            m_pixelShader2D = nullptr;
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
        m_2DDrawBatches.clear();
        m_3DDrawBatches.clear();
    }
}

#endif
