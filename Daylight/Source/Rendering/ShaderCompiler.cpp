#include "pch.h"
#include "ShaderCompiler.h"

namespace Dlight
{
	ShaderCompiler::ShaderCompiler()
	{
		// 파일 읽기 등 보조 기능을 제공하는 DXC 객체를 생성한다.
		HRESULT result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf()));
		if (FAILED(result))
		{
			DL_LOG_ERROR("Failed to create DXC utils. HRESULT: ", result);
			std::abort();
		}

		// HLSL을 실제로 컴파일할 객체를 생성한다.
		result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.GetAddressOf()));
		if (FAILED(result))
		{
			DL_LOG_ERROR("Failed to create DXC compiler. HRESULT: ", result);
			std::abort();
		}

		// 셰이더의 #include 파일을 읽을 기본 처리기를 생성한다.
		result = utils->CreateDefaultIncludeHandler(includeHandler.GetAddressOf());
		if (FAILED(result))
		{
			DL_LOG_ERROR("Failed to create DXC include handler. HRESULT: ", result);
			std::abort();
		}
	}

	// ComPtr 멤버가 소멸하면서 DXC 객체를 자동으로 해제한다.
	ShaderCompiler::~ShaderCompiler() = default;

	std::vector<uint32> ShaderCompiler::Compile(
		const wchar_t* filePath,
		const wchar_t* entryPoint,
		const wchar_t* targetProfile)
	{
		// 파일 경로, 진입 함수 이름, 셰이더 프로파일이 비어 있는지 확인한다.
		if (!filePath || !*filePath || !entryPoint || !*entryPoint || !targetProfile || !*targetProfile)
		{
			DL_LOG_ERROR("Shader compilation requires a file path, entry point and target profile");
			std::abort();
		}

		// 로그에 파일 경로를 출력할 수 있도록 wide 문자열을 UTF-8로 변환한다.
		const std::wstring shaderPath(filePath);
		ComPtr<IDxcBlobEncoding> pathBlob;
		HRESULT result = utils->CreateBlob(shaderPath.c_str(),
			static_cast<uint32>((shaderPath.size() + 1) * sizeof(wchar_t)),
			DXC_CP_WIDE, pathBlob.GetAddressOf());
		
		ComPtr<IDxcBlobUtf8> pathUtf8;
		if (FAILED(result) || FAILED(utils->GetBlobAsUtf8(pathBlob.Get(), pathUtf8.GetAddressOf())))
		{
			DL_LOG_ERROR("Failed to convert shader file path to UTF-8");
			std::abort();
		}
		const char* logPath = pathUtf8->GetStringPointer();

		// HLSL 파일 내용을 메모리로 읽는다. 기본 인코딩은 UTF-8이다.
		ComPtr<IDxcBlobEncoding> source;
		uint32 codePage = DXC_CP_UTF8;
		result = utils->LoadFile(filePath, &codePage, source.GetAddressOf());
		if (FAILED(result))
		{
			DL_LOG_ERROR("Failed to load shader: ", logPath, ". HRESULT: ", result);
			std::abort();
		}

		// 읽은 소스의 주소, 크기, 인코딩을 DXC에 전달할 형태로 묶는다.
		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = source->GetBufferPointer();
		sourceBuffer.Size = source->GetBufferSize();
		sourceBuffer.Encoding = codePage;

		// 셰이더가 있는 폴더를 #include 검색 경로에 추가한다.
		const size_t separator = shaderPath.find_last_of(L"/\\");
		const std::wstring includeDirectory = separator == std::wstring::npos
			? L"." : shaderPath.substr(0, separator + 1);
		// 진입 함수(-E), 프로파일(-T), SPIR-V 출력과 대상 Vulkan 환경을 지정한다.
		// Y 반전은 viewport에서 처리하므로 컴파일 옵션에 넣지 않는다.
		const wchar_t* arguments[] = {
			filePath,
			L"-E", entryPoint,
			L"-T", targetProfile,
			L"-spirv",
			L"-fspv-target-env=vulkan1.3",
			L"-I", includeDirectory.c_str()
		};

		// DXC를 호출한다. 호출 성공 여부와 셰이더 컴파일 성공 여부는 별개이다.
		ComPtr<IDxcResult> compileResult;
		result = compiler->Compile(&sourceBuffer, arguments,
			static_cast<uint32>(sizeof(arguments) / sizeof(arguments[0])),
			includeHandler.Get(), IID_PPV_ARGS(compileResult.GetAddressOf()));
		if (FAILED(result))
		{
			DL_LOG_ERROR("DXC invocation failed for shader: ", logPath, ". HRESULT: ", result);
			std::abort();
		}

		// 실제 셰이더 컴파일이 성공했는지 결과에서 확인한다.
		HRESULT compileStatus = S_OK;
		result = compileResult->GetStatus(&compileStatus);
		if (FAILED(result))
		{
			DL_LOG_ERROR("Failed to read compilation status for shader: ", logPath, ". HRESULT: ", result);
			std::abort();
		}

		// 경고 또는 오류 내용을 먼저 출력하고, 컴파일 실패라면 종료한다.
		ComPtr<IDxcBlobUtf8> diagnostics;
		result = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(diagnostics.GetAddressOf()), nullptr);
		if (FAILED(result))
		{
			DL_LOG_ERROR("Failed to read DXC diagnostics for shader: ", logPath, ". HRESULT: ", result);
			std::abort();
		}
		if (diagnostics && diagnostics->GetStringLength() != 0)
		{
			if (FAILED(compileStatus))
				DL_LOG_ERROR(logPath, ": ", diagnostics->GetStringPointer());
			else
				DL_LOG_WARN(logPath, ": ", diagnostics->GetStringPointer());
		}
		if (FAILED(compileStatus))
		{
			DL_LOG_ERROR("Shader compilation failed: ", logPath, ". HRESULT: ", compileStatus);
			std::abort();
		}

		// 생성된 SPIR-V를 가져오고 uint32 배열로 담을 수 있는 크기인지 확인한다.
		ComPtr<IDxcBlob> object;
		result = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(object.GetAddressOf()), nullptr);
		if (FAILED(result) || !object || object->GetBufferSize() == 0 || object->GetBufferSize() % sizeof(uint32) != 0)
		{
			DL_LOG_ERROR("Failed to obtain SPIR-V for shader: ", logPath, ". HRESULT: ", result);
			std::abort();
		}

		// DXC의 임시 Blob이 해제되어도 결과가 유지되도록 벡터에 복사해 반환한다.
		std::vector<uint32> spirv(object->GetBufferSize() / sizeof(uint32));
		std::memcpy(spirv.data(), object->GetBufferPointer(), object->GetBufferSize());
		return spirv;
	}
}
