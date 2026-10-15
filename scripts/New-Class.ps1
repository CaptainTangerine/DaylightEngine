[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$ClassPath
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($ClassPath)) {
    Write-Host 'Usage: .\new-class <path relative to Daylight/Source>'
    Write-Host '  .\new-class Scene/Entity'
    Write-Host '  .\new-class Graphics/Vulkan/VulkanTexture'
    exit 0
}

$createdFiles = [System.Collections.Generic.List[string]]::new()

try {
    $projectRoot = Split-Path -Parent $PSScriptRoot
    $sourceRoot = [System.IO.Path]::GetFullPath((Join-Path $projectRoot 'Daylight/Source'))
    $sourcePrefix = $sourceRoot.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar

    if ([System.IO.Path]::IsPathRooted($ClassPath)) {
        throw 'Use a path relative to Daylight/Source.'
    }

    $classBasePath = [System.IO.Path]::GetFullPath((Join-Path $sourceRoot $ClassPath))
    if (-not $classBasePath.StartsWith($sourcePrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw 'The output path must stay inside Daylight/Source.'
    }

    $className = [System.IO.Path]::GetFileName($classBasePath)
    if ($className -cnotmatch '^[A-Za-z][A-Za-z0-9_]*$') {
        throw 'Use a C++ class name such as Entity or MeshComponent, without a file extension.'
    }
    if ($className -match '^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$') {
        throw 'This name is reserved by Windows.'
    }

    $headerPath = $classBasePath + '.h'
    $cppPath = $classBasePath + '.cpp'
    foreach ($filePath in @($headerPath, $cppPath)) {
        if (Test-Path -LiteralPath $filePath) {
            throw "Already exists; no files were changed: $filePath"
        }
    }

    $header = @"
#pragma once

namespace Dlight
{
    class $className
    {
    public:
        ${className}();
        ~${className}();

    private:
    };
}
"@

    $cpp = @"
#include "pch.h"
#include "${className}.h"

namespace Dlight
{
    ${className}::${className}()
    {
    }

    ${className}::~${className}()
    {
    }
}
"@

    $outputDirectory = [System.IO.Path]::GetDirectoryName($classBasePath)
    [System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
    $encoding = [System.Text.UTF8Encoding]::new($false)
    $outputs = @(
        @{ Path = $headerPath; Content = $header },
        @{ Path = $cppPath; Content = $cpp }
    )

    foreach ($output in $outputs) {
        # CreateNew also protects existing files if another process creates one now.
        $stream = [System.IO.File]::Open(
            $output.Path, [System.IO.FileMode]::CreateNew,
            [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
        $createdFiles.Add($output.Path)
        try {
            $content = $output.Content.Replace("`r`n", "`n").Replace("`n", "`r`n") + "`r`n"
            $bytes = $encoding.GetBytes($content)
            $stream.Write($bytes, 0, $bytes.Length)
        }
        finally {
            $stream.Dispose()
        }
    }

    foreach ($filePath in $createdFiles) {
        Write-Host "Created: $filePath"
    }
}
catch {
    # Roll back only the files created by this invocation, never pre-existing files.
    foreach ($filePath in $createdFiles) {
        Remove-Item -LiteralPath $filePath -ErrorAction SilentlyContinue
    }
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
