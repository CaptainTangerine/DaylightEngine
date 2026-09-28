#pragma once

#include <dxc/dxcapi.h>
#include <string>

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
			const std::string& entryPoint,
			const wchar_t* targetProfile);

	private:
		void Initialize();

	private:
		ComPtr<IDxcUtils> utils;
		ComPtr<IDxcCompiler3> compiler;
		ComPtr<IDxcIncludeHandler> includeHandler;
	};
}

