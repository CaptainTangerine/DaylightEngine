#include "pch.h"
#include "GfxDevice.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

bool GfxDevice::Init(const GfxDeviceDesc& desc)
{
	width  = desc.width;
	height = desc.height;

	// 스왑체인
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width  = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator   = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.SampleDesc.Count   = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount  = 2;
	sd.OutputWindow = desc.windowHandle;
	sd.Windowed     = TRUE;
	sd.SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	UINT createFlags = 0;
	if (desc.debugLayer)
	{
		createFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

	// 디바이스 + 컨텍스트 + 스왑체인을 한번에 생성
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createFlags,
		&featureLevel, 1,
		D3D11_SDK_VERSION,
		&sd,
		swapChain.GetAddressOf(),
		device.GetAddressOf(),
		nullptr,
		context.GetAddressOf()
	);

	if (FAILED(hr)) return false;

	return CreateBackbufferRTV();
}

void GfxDevice::Shutdown()
{
	backbufferRTV.Reset();
	swapChain.Reset();
	context.Reset();
	device.Reset();
}

void GfxDevice::BeginFrame()
{
	float clearColor[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	context->ClearRenderTargetView(backbufferRTV.Get(), clearColor);
	context->OMSetRenderTargets(1, backbufferRTV.GetAddressOf(), nullptr);

	D3D11_VIEWPORT vp = {};
	vp.Width    = static_cast<float>(width);
	vp.Height   = static_cast<float>(height);
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);
}

void GfxDevice::EndFrame(bool vsync)
{
	swapChain->Present(vsync ? 1 : 0, 0);
}

void GfxDevice::Resize(int w, int h)
{
	if (w <= 0 || h <= 0) return;
	if (w == width && h == height) return;

	width  = w;
	height = h;

	// 백버퍼를 참조하는 뷰를 먼저 놓아줘야 ResizeBuffers가 성공한다
	context->OMSetRenderTargets(0, nullptr, nullptr);
	backbufferRTV.Reset();

	swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	CreateBackbufferRTV();
}

bool GfxDevice::CreateBackbufferRTV()
{
	ComPtr<ID3D11Texture2D> backbuffer;
	HRESULT hr = swapChain->GetBuffer(
		0, __uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(backbuffer.GetAddressOf())
	);
	if (FAILED(hr)) return false;

	hr = device->CreateRenderTargetView(
		backbuffer.Get(), nullptr,
		backbufferRTV.GetAddressOf()
	);
	return SUCCEEDED(hr);
}

ComPtr<ID3D11Buffer> GfxDevice::CreateVertexBuffer(const void* data, UINT byteSize)
{
	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = byteSize;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = data;

	ComPtr<ID3D11Buffer> buffer;
	HRESULT hr = device->CreateBuffer(&bd, &initData, buffer.GetAddressOf());
	if (FAILED(hr))
	{
		return nullptr;
	}

	return buffer;
}

ComPtr<ID3D11InputLayout> GfxDevice::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* elements, UINT count, ID3DBlob* vsBytecode)
{
	ComPtr<ID3D11InputLayout> layout;
	HRESULT hr = device->CreateInputLayout(
		elements, count,
		vsBytecode->GetBufferPointer(),
		vsBytecode->GetBufferSize(),
		layout.GetAddressOf()
	);

	if (FAILED(hr))
	{
		return nullptr;
	}
	return layout;
}
