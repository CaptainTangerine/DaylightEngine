#pragma once

#include <dxc/dxcapi.h>

namespace Dlight
{
	class ShaderCompiler
	{
	public:
		ShaderCompiler();
		~ShaderCompiler();

		ShaderCompiler(const ShaderCompiler&) = delete;
		ShaderCompiler& operator=(const ShaderCompiler&) = delete;

		std::vector<uint32> Compile(
			const wchar_t* filePath,
			const wchar_t* entryPoint,
			const wchar_t* targetProfile);

	private:
		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		ComPtr<IDxcIncludeHandler> includeHandler;
	};
}

