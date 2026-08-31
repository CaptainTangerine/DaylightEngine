#include "pch.h"
#include "Renderer.h"

bool Renderer::Init(GfxDevice& GfxDevice)
{
    if (!vertexShader.CompileFromFile(GfxDevice.GetDevice(), L"Shaders/Triangle.hlsl", "VS_Main", GfxShaderStage::Vertex))
    {
        return false;
    }

    if (!pixelShader.CompileFromFile(GfxDevice.GetDevice(), L"Shaders/Triangle.hlsl", "PS_Main", GfxShaderStage::Pixel))
    {
        return false;
    }

    struct Vertex
    {
        float position[3];
        float color[3];
    };

    Vertex vertices[] =
    {
        { {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
    };

    vertexBuffer = GfxDevice.CreateVertexBuffer(vertices, sizeof(vertices));
    if (!vertexBuffer) return false;

    // 3) 입력 레이아웃
    D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    inputLayout = GfxDevice.CreateInputLayout(layoutDesc, 2, vertexShader.GetBytecode());
    if (!inputLayout) return false;

    return true;

}

void Renderer::ShutDown()
{
    inputLayout.Reset();
    vertexBuffer.Reset();
}

void Renderer::Render(GfxDevice& GfxDevice)
{
    CheckShaderReload(GfxDevice);

    ID3D11DeviceContext* ctx = GfxDevice.GetContext();

    ctx->IASetInputLayout(inputLayout.Get());

    UINT stride = 24;  // float[3] + float[3] = 24바이트
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    vertexShader.Bind(ctx);
    pixelShader.Bind(ctx);

    ctx->Draw(3, 0);
}

void Renderer::CheckShaderReload(GfxDevice& gfx)
{
    std::error_code ec;
    auto currentTime = std::filesystem::last_write_time(L"Shaders/Triangle.hlsl", ec);

    if (ec)
    {
        return;
    }

    // 변경 없음
    if (currentTime == lastShaderWriteTime)
    {
        return; 
    }
    lastShaderWriteTime = currentTime;

    vertexShader.Recompile(gfx.GetDevice());
    pixelShader.Recompile(gfx.GetDevice());
}
