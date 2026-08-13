#ifdef _WIN32

#include "D3D11Renderer.h"

#include <d3dcompiler.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

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

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0f);
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
                "JevaingBigBearGummy2D",
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

    D3D11Renderer::D3D11Renderer(RendererTestPattern testPattern)
        : m_testPattern(testPattern)
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

        Logger::Info("DirectX 11 BIG BEAR GUMMY 2D pipeline initialized.");
        return true;
    }

    bool D3D11Renderer::Create2DPipeline()
    {
        ID3DBlob* vertexShaderBlob = nullptr;
        ID3DBlob* pixelShaderBlob = nullptr;

        if (!CompileShader("VSMain", "vs_5_0", &vertexShaderBlob))
        {
            return false;
        }

        if (!CompileShader("PSMain", "ps_5_0", &pixelShaderBlob))
        {
            vertexShaderBlob->Release();
            return false;
        }

        const HRESULT vertexShaderResult = m_device->CreateVertexShader(
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            nullptr,
            &m_vertexShader
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
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(),
            &m_inputLayout
        );

        pixelShaderBlob->Release();
        vertexShaderBlob->Release();

        if (
            FAILED(vertexShaderResult) ||
            FAILED(pixelShaderResult) ||
            FAILED(inputLayoutResult) ||
            !m_vertexShader ||
            !m_pixelShader ||
            !m_inputLayout
        )
        {
            Logger::Error("DirectX 11 failed to create the BIG BEAR GUMMY shader pipeline.");
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

        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;
        m_viewport.Width = static_cast<float>(width);
        m_viewport.Height = static_cast<float>(height);
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;

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
            Logger::Error("DirectX 11 failed to allocate the dynamic 2D vertex buffer.");
            m_vertexCapacity = 0;
            return false;
        }

        m_vertexCapacity = newCapacity;
        return true;
    }

    void D3D11Renderer::BeginFrame()
    {
        m_frameVertices.clear();

        if (!m_context || !m_renderTargetView)
        {
            return;
        }

        m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
        m_context->RSSetViewports(1, &m_viewport);
        Clear(DefaultClear);
        AppendTestPattern();
    }

    void D3D11Renderer::EndFrame()
    {
        FlushDrawCommands();

        if (m_swapChain)
        {
            m_swapChain->Present(1, 0);
        }
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
        m_frameVertices.push_back(MakeVertex(a.X, a.Y, color));
        m_frameVertices.push_back(MakeVertex(b.X, b.Y, color));
        m_frameVertices.push_back(MakeVertex(c.X, c.Y, color));
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
        }

        m_frameVertices.insert(
            m_frameVertices.end(),
            vertices.begin(),
            vertices.end()
        );
    }

    void D3D11Renderer::FlushDrawCommands()
    {
        if (m_frameVertices.empty() || !m_context)
        {
            return;
        }

        if (!EnsureVertexCapacity(m_frameVertices.size()))
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
            m_frameVertices.data(),
            m_frameVertices.size() * sizeof(D3D11Vertex)
        );
        m_context->Unmap(m_vertexBuffer, 0);

        constexpr UINT stride = sizeof(D3D11Vertex);
        constexpr UINT offset = 0;

        m_context->IASetInputLayout(m_inputLayout);
        m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->VSSetShader(m_vertexShader, nullptr, 0);
        m_context->PSSetShader(m_pixelShader, nullptr, 0);
        m_context->Draw(static_cast<UINT>(m_frameVertices.size()), 0);
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

        if (m_vertexShader)
        {
            m_vertexShader->Release();
            m_vertexShader = nullptr;
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
        m_frameVertices.clear();
    }
}

#endif
