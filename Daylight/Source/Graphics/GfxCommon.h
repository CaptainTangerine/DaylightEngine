#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <d3dcompiler.h>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
