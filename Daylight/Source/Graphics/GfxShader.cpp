#include "pch.h"
#include "GfxShader.h"

#pragma comment(lib, "d3dcompiler.lib")

bool GfxShader::CompileFromFile(ID3D11Device* device, const wchar_t* path, const char* entryPoint, GfxShaderStage _stage)
{
    stage = _stage;

    const char* target = (stage == GfxShaderStage::Vertex) ? "vs_5_0" : "ps_5_0";

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // 3) 컴파일
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompileFromFile(
        path,        
        nullptr,     
        nullptr,     
        entryPoint,  
        target,      
        flags,
        0,              
        bytecode.GetAddressOf(),
        errorBlob.GetAddressOf()
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    if (stage == GfxShaderStage::Vertex)
    {
        hr = device->CreateVertexShader(
            bytecode->GetBufferPointer(),
            bytecode->GetBufferSize(),
            nullptr,
            vertexShader.GetAddressOf()
        );
    }
    else if (stage == GfxShaderStage::Pixel)
    {
        hr = device->CreatePixelShader(
            bytecode->GetBufferPointer(),
            bytecode->GetBufferSize(),
            nullptr,
            pixelShader.GetAddressOf()
        );
    }

    return SUCCEEDED(hr);
}

void GfxShader::Bind(ID3D11DeviceContext* context) const
{
    if (stage == GfxShaderStage::Vertex)
    {
        context->VSSetShader(vertexShader.Get(), nullptr, 0);
    }
    else if (stage == GfxShaderStage::Pixel)
    {
        context->PSSetShader(pixelShader.Get(), nullptr, 0);
    }
}
