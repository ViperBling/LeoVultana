#include "PCH.h"
#include "ShaderCompilerCache.h"

std::string sShaderLibDir;
std::string sShaderCacheDir;

bool InitShaderCompilerCache(const std::string shaderLibDir, std::string shaderCacheDir)
{
    sShaderLibDir = shaderLibDir;
    sShaderCacheDir = shaderCacheDir;

    return true;
}

std::string GetShaderCompilerLibDir()
{
    return sShaderLibDir;
}

std::string GetShaderCompilerCacheDir()
{
    return sShaderCacheDir;
}