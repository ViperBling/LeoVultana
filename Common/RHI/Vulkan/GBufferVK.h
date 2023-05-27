#pragma once

#include "DeviceVK.h"
#include "SwapChainVK.h"
#include "TextureVK.h"
#include "ExtDebugUtilsVK.h"
#include "ShaderCompilerHelperVK.h"
#include "ResourceViewHeapsVK.h"

namespace LeoVultana_VK
{
    typedef enum GBufferFlagBits
    {
        GBUFFER_NONE = 0,
        GBUFFER_DEPTH = 1,
        GBUFFER_FORWARD = 2,
        GBUFFER_MOTION_VECTORS = 4,
        GBUFFER_NORMAL_BUFFER = 8,
        GBUFFER_DIFFUSE = 16,
        GBUFFER_SPECULAR_ROUGHNESS = 32
    } GBufferFlagBits;

    typedef uint32_t GBufferFlags;

    class GBuffer;

    class GBufferRenderPass
    {

    };

    class GBuffer
    {

    };
}