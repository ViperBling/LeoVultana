#pragma once

#include "PCHVK.h"

namespace LeoVultana_VK
{
    class InstanceProperties
    {
    public:
        void Init();
        bool AddInstanceLayerName(const char* instLayerName);
        bool AddInstanceExtensionName(const char* instExtName);
        void * GetNext() { return m_pNext; }
        void SetNewNext(void * pNext) { m_pNext = pNext; }

        void GetExtensionNamesAndConfigs(std::vector<const char*>* pInstLayerName, std::vector<const char*>* pInstExtName);

    private:
        bool IsLayerPresent(const char* pLayerName);
        bool IsExtensionPresent(const char* pExtensionName);
    private:
        std::vector<VkLayerProperties> mInstanceLayerProperties;
        std::vector<VkExtensionProperties> mInstanceExtensionProperties;

        std::vector<const char*> mInstanceLayerNames;
        std::vector<const char*> mInstanceExtensionNames;
        void* m_pNext = nullptr;
    };
}