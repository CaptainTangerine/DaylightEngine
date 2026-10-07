#pragma once

#include <filesystem>

namespace Dlight::Paths
{
    const std::filesystem::path& GetExecutableDirectory();
    std::filesystem::path GetShaderPath(const std::filesystem::path& relativePath);
    std::filesystem::path GetAssetPath(const std::filesystem::path& relativePath);
}
