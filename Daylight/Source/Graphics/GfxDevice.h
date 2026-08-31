#pragma once
#include "GfxCommon.h"

struct GfxDeviceDesc
{
	HWND windowHandle = nullptr;
	int  width  = 0;
	int  height = 0;
	bool debugLayer = false;
};

class GfxDevice
{
public:
	GfxDevice() = default;
	~GfxDevice() = default;

	GfxDevice(const GfxDevice&) = delete;
	GfxDevice& operator=(const GfxDevice&) = delete;

public:
	bool Init(const GfxDeviceDesc& desc);
	void Shutdown();

	void BeginFrame();
	void EndFrame(bool vsync = true);

	void Resize(int width, int height);

public:
	ID3D11Device*		 GetDevice()  const { return device.Get(); }
	ID3D11DeviceContext* GetContext() const { return context.Get(); }

	int GetWidth()  const { return width; }
	int GetHeight() const { return height; }

public:
	ComPtr<ID3D11Buffer> CreateVertexBuffer(const void* data, UINT byteSize);

	ComPtr<ID3D11InputLayout> CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* elements,
		UINT count,
		ID3DBlob* vsBytecode);

private:
	bool CreateBackbufferRTV();


private:
	ComPtr<ID3D11Device>		   device;
	ComPtr<ID3D11DeviceContext>	   context;
	ComPtr<IDXGISwapChain>		   swapChain;
	ComPtr<ID3D11RenderTargetView> backbufferRTV;

	int width  = 0;
	int height = 0;
};
