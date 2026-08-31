#pragma once
#include "Graphics/GfxShader.h"
#include "Graphics/GfxDevice.h"

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

public:
	bool Init(GfxDevice& GfxDevice);
	void ShutDown();

	void Render(GfxDevice& GfxDevice);

private:
	void CheckShaderReload(GfxDevice& gfx);

private:
	GfxShader vertexShader;
	GfxShader pixelShader;

	ComPtr<ID3D11Buffer>      vertexBuffer;
	ComPtr<ID3D11InputLayout> inputLayout;

	std::filesystem::file_time_type lastShaderWriteTime;
};

