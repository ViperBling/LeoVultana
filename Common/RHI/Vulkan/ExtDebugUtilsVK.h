#pragma once

#include "Vulkan/vulkan.h"
#include "InstancePropertiesVK.h"

namespace LeoVultana_VK
{
    void ExtDebugUtilsGetProAddresses(VkDevice device);
    bool ExtDebugUtilsCheckInstanceExtensions(InstanceProperties* pInstanceProp);
    void SetResourceName(VkDevice device, VkObjectType objectType, uint64_t handle, const char* name);
    void SetPerfMarkerBegin(VkCommandBuffer cmdBuffer, const char* name);
    void SetPerfMarkerEnd(VkCommandBuffer cmdBuffer);
}