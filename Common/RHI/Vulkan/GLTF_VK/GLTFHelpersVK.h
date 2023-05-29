#pragma once

#include <Vulkan/vulkan.h>
#include "GLTFHelpers.h"

namespace LeoVultana_VK
{
    VkFormat GetFormat(const std::string& str, int id);
    uint32_t SizeOfFormat(VkFormat format);
}