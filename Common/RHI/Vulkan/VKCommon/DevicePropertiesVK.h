#pragma once

#include "PCHVK.h"

namespace LeoVultana_VK
{
    class DeviceProperties
    {
    public:
        void Init(VkPhysicalDevice physicalDevice);
        bool AddDeviceExtensionName(const char* deviceExtensionName);

        void * GetNext() { return m_pNext; }
        void SetNewNext(void* pNext) { m_pNext = pNext; }

        VkPhysicalDevice GetPhysicalDevice() { return mPhysicalDevice; }
        void GetExtensionNamesAndConfigs(std::vector<const char*>* pDeviceExtName);

    private:
        bool IsExtensionPresent(const char* pExtName);

    private:
        VkPhysicalDevice mPhysicalDevice;
        std::vector<const char*> mDeviceExtNames;
        std::vector<VkExtensionProperties> mDeviceExtProperties;
        void* m_pNext{};
    };
}