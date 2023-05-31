#pragma once

#include "DeviceVK.h"
#include "Utilities/ShaderCompiler.h"
#include "Utilities/DXCHelper.h"

class Sync;

namespace LeoVultana_VK
{
    enum ShaderSourceType
    {
        SST_HLSL,
        SST_GLSL
    };

    void CreateShaderCache();
    void DestroyShaderCache(Device* pDevice);

    // Does as the function name says and uses a cache
    VkResult VKCompileFromString(
        VkDevice device,
        ShaderSourceType sourceType,
        VkShaderStageFlagBits shader_type,
        const char *pShaderCode,
        const char *pShaderEntryPoint,
        const char *pExtraParams,
        const DefineList *pDefines,
        VkPipelineShaderStageCreateInfo *pShader);

    VkResult VKCompileFromFile(
        VkDevice device,
        VkShaderStageFlagBits shader_type,
        const char *pFilename,
        const char *pShaderEntryPoint,
        const char *pExtraParams,
        const DefineList *pDefines,
        VkPipelineShaderStageCreateInfo *pShader);
}