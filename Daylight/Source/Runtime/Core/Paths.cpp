#include "pch.h"
#include "Paths.h"

#include <Windows.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <stdexcept>

namespace Dlight::Paths
{
    static std::wstring ConvertUtf8ToWide(const char* utf8Text)
    {
        // 변환된 문자열을 저장하는 데 필요한 크기를 먼저 구한다.
        const int requiredSize = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, utf8Text, -1, nullptr, 0);
        if (requiredSize == 0)
        {
            throw std::runtime_error("Failed to determine UTF-16 path length");
        }

        std::wstring wideText(static_cast<size_t>(requiredSize), L'\0');

        // 확보한 문자열 공간에 UTF-8 경로를 UTF-16으로 변환한다.
        const int convertedSize = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, utf8Text, -1,
            wideText.data(), requiredSize);
        if (convertedSize == 0)
        {
            throw std::runtime_error("Failed to convert UTF-8 path to UTF-16");
        }

        // 변환 결과에 포함된 끝의 널 문자를 문자열 길이에서 제외한다.
        wideText.resize(static_cast<size_t>(convertedSize - 1));
        return wideText;
    }

    static std::filesystem::path LoadExecutableDirectory()
    {
        const char* utf8Directory = SDL_GetBasePath();
        if (!utf8Directory)
        {
            throw std::runtime_error(
                std::string("Failed to get application directory: ") + SDL_GetError());
        }

        const std::wstring wideDirectory = ConvertUtf8ToWide(utf8Directory);
        return std::filesystem::path(wideDirectory);
    }

    const std::filesystem::path& GetExecutableDirectory()
    {
        static const std::filesystem::path directory = LoadExecutableDirectory();
        return directory;
    }

    std::filesystem::path GetShaderPath(const std::filesystem::path& relativePath)
    {
        return GetExecutableDirectory() / L"Shaders" / relativePath;
    }

    std::filesystem::path GetAssetPath(const std::filesystem::path& relativePath)
    {
        return GetExecutableDirectory() / L"Assets" / relativePath;
    }
}
