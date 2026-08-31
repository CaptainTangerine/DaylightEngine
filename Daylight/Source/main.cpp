#include "pch.h"
#include "Platform/Window.h"
#include "Graphics/GfxDevice.h"

#include "Graphics/GfxShader.h"

int APIENTRY wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    Window window(hInstance, L"Daylight Engine", 1280, 720);

    GfxDevice gfx;

    GfxDeviceDesc desc;
    desc.windowHandle = window.GetHandle();
    desc.width  = 1280;
    desc.height = 720;

#if defined(_DEBUG)
    desc.debugLayer = true;
#endif

    if (!gfx.Init(desc))
    {
        return -1;
    }

    DelegateHandle resizeHandle = window.onResize.Add(
        [&gfx](int w, int h)
        {
            gfx.Resize(w, h);
        }
    );


    GfxShader vs, ps;
    vs.CompileFromFile(gfx.GetDevice(), L"Shaders/Triangle.hlsl", "VS_Main", GfxShaderStage::Vertex);
    ps.CompileFromFile(gfx.GetDevice(), L"Shaders/Triangle.hlsl", "PS_Main", GfxShaderStage::Pixel);

    struct Vertex
    {
        float position[3];
        float color[3];
    };

    Vertex vertices[] =
    {
        { {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },  // 꼭대기 - 빨강
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },  // 오른쪽 아래 - 초록
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },  // 왼쪽 아래 - 파랑
    };

    auto vertexBuffer = gfx.CreateVertexBuffer(vertices, sizeof(vertices));

    // 3) 입력 레이아웃
    D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    auto inputLayout = gfx.CreateInputLayout(layoutDesc, 2, vs.GetBytecode());

    while (window.ProcessMessages())
    {
        gfx.BeginFrame();

        ID3D11DeviceContext* ctx = gfx.GetContext();

        ctx->IASetInputLayout(inputLayout.Get());

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        vs.Bind(ctx);
        ps.Bind(ctx);

        ctx->Draw(3, 0);

        gfx.EndFrame(true);
    }

    window.onResize.Remove(resizeHandle);
    gfx.Shutdown();

    return 0;
}
