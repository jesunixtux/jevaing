#ifdef _WIN32

#include "D3D11Renderer.h"

#include <d3dcompiler.h>

#include <cstddef>
#include <string>

#include "../../Core/Logger.h"
#include "../../Core/Window.h"

namespace Jevaing::Internal
{
    namespace
    {
        struct Vertex
        {
            float Position[3];
            float Color[4];
        };

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
                "JevaingAtlasShader",
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

        ID3D11Texture2D* backBuffer = nullptr;
        const HRESULT bufferResult = m_swapChain->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(&backBuffer)
        );

        if (FAILED(bufferResult) || !backBuffer)
        {
            Logger::Error("DirectX 11 failed to acquire the swap-chain back buffer.");
            ReleaseResources();
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
            ReleaseResources();
            return false;
        }

        if (!CreateTrianglePipeline(hwnd))
        {
            ReleaseResources();
            return false;
        }

        Logger::Info("DirectX 11 device, swap chain and ATLAS triangle pipeline initialized.");
        return true;
    }

    bool D3D11Renderer::CreateTrianglePipeline(HWND hwnd)
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

        if (FAILED(vertexShaderResult) || !m_vertexShader)
        {
            Logger::Error("DirectX 11 failed to create the ATLAS vertex shader.");
            pixelShaderBlob->Release();
            vertexShaderBlob->Release();
            return false;
        }

        const HRESULT pixelShaderResult = m_device->CreatePixelShader(
            pixelShaderBlob->GetBufferPointer(),
            pixelShaderBlob->GetBufferSize(),
            nullptr,
            &m_pixelShader
        );

        if (FAILED(pixelShaderResult) || !m_pixelShader)
        {
            Logger::Error("DirectX 11 failed to create the ATLAS pixel shader.");
            pixelShaderBlob->Release();
            vertexShaderBlob->Release();
            return false;
        }

        const D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            {
                "POSITION",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                static_cast<UINT>(offsetof(Vertex, Position)),
                D3D11_INPUT_PER_VERTEX_DATA,
                0
            },
            {
                "COLOR",
                0,
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                0,
                static_cast<UINT>(offsetof(Vertex, Color)),
                D3D11_INPUT_PER_VERTEX_DATA,
                0
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

        if (FAILED(inputLayoutResult) || !m_inputLayout)
        {
            Logger::Error("DirectX 11 failed to create the ATLAS input layout.");
            return false;
        }

        constexpr Vertex vertices[] = {
            {
                { 0.0f, 0.62f, 0.0f },
                { 1.0f, 0.25f, 0.16f, 1.0f }
            },
            {
                { 0.62f, -0.58f, 0.0f },
                { 0.15f, 0.72f, 1.0f, 1.0f }
            },
            {
                { -0.62f, -0.58f, 0.0f },
                { 0.35f, 1.0f, 0.35f, 1.0f }
            }
        };

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(vertices));
        vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = vertices;

        const HRESULT vertexBufferResult = m_device->CreateBuffer(
            &vertexBufferDesc,
            &vertexData,
            &m_vertexBuffer
        );

        if (FAILED(vertexBufferResult) || !m_vertexBuffer)
        {
            Logger::Error("DirectX 11 failed to create the ATLAS vertex buffer.");
            return false;
        }

        RECT clientRect = {};

        if (!GetClientRect(hwnd, &clientRect))
        {
            Logger::Error("DirectX 11 failed to query the Win32 client area for the viewport.");
            return false;
        }

        const LONG clientWidth = clientRect.right - clientRect.left;
        const LONG clientHeight = clientRect.bottom - clientRect.top;

        if (clientWidth <= 0 || clientHeight <= 0)
        {
            Logger::Error("DirectX 11 received an invalid viewport size.");
            return false;
        }

        m_viewport.TopLeftX = 0.0f;
        m_viewport.TopLeftY = 0.0f;
        m_viewport.Width = static_cast<float>(clientWidth);
        m_viewport.Height = static_cast<float>(clientHeight);
        m_viewport.MinDepth = 0.0f;
        m_viewport.MaxDepth = 1.0f;

        return true;
    }

    void D3D11Renderer::BeginFrame()
    {
        if (
            !m_context ||
            !m_renderTargetView ||
            !m_vertexShader ||
            !m_pixelShader ||
            !m_inputLayout ||
            !m_vertexBuffer
        )
        {
            return;
        }

        constexpr float clearColor[4] = {
            0.035f,
            0.055f,
            0.085f,
            1.0f
        };

        m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
        m_context->ClearRenderTargetView(m_renderTargetView, clearColor);
        m_context->RSSetViewports(1, &m_viewport);

        constexpr UINT stride = sizeof(Vertex);
        constexpr UINT offset = 0;

        m_context->IASetInputLayout(m_inputLayout);
        m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->VSSetShader(m_vertexShader, nullptr, 0);
        m_context->PSSetShader(m_pixelShader, nullptr, 0);
        m_context->Draw(3, 0);
    }

    void D3D11Renderer::EndFrame()
    {
        if (m_swapChain)
        {
            m_swapChain->Present(1, 0);
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
    }
}

#endif
