#pragma once
#include "GfxCommon.h"

enum class GfxShaderStage
{
	Vertex,
	Pixel
};


class GfxShader
{
public:
	GfxShader() = default;
	~GfxShader() = default;

public:
	bool CompileFromFile(ID3D11Device* device,
		const wchar_t* path,
		const char* entryPoint,
		GfxShaderStage _stage);

	void Bind(ID3D11DeviceContext* context) const;


	ID3DBlob* GetBytecode() const { return bytecode.Get(); }

private:
	GfxShaderStage stage = GfxShaderStage::Vertex;

	ComPtr<ID3DBlob>           bytecode;
	ComPtr<ID3D11VertexShader> vertexShader;
	ComPtr<ID3D11PixelShader>  pixelShader;

};