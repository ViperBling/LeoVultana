#pragma once

#include "PCHVK.h"
#include "InstancePropertiesVK.h"

namespace LeoVultana_VK
{
    // 创建实例
    bool CreateInstance(const char* pAppName, const char* pEngineName, VkInstance* pVulkanInst, VkPhysicalDevice* pPhysicalDevice, InstanceProperties* pInstProps);
    VkInstance CreateInstance(VkApplicationInfo appInfo, InstanceProperties* pInstProps);
    void DestroyInstance(VkInstance instance);
}