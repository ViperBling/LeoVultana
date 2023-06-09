#include "InstanceVK.h"
#include "ExtValidationVK.h"
#include "ExtFreeSyncHDRVK.h"
#include "ExtDebugUtilsVK.h"
#include "HelperVK.h"

namespace LeoVultana_VK
{
    uint32_t GetScore(VkPhysicalDevice physicalDevice)
    {
        uint32_t score = 0;

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        switch (deviceProperties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score += 1000;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score += 10000;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score += 100;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score += 10;
                break;
            default:
                break;
        }
        return score;
    }

    VkPhysicalDevice SelectPhysicalDevice(std::vector<VkPhysicalDevice>& physicalDevices)
    {
        assert(!physicalDevices.empty() && "No GPU Found.");

        std::multimap<uint32_t, VkPhysicalDevice> ratings;

        for (auto it = physicalDevices.begin(); it != physicalDevices.end(); ++it)
        {
            ratings.insert(std::make_pair(GetScore(*it), *it));
        }
        return ratings.rbegin()->second;
    }

    bool CreateInstance(
        const char *pAppName, const char *pEngineName, VkInstance *pVulkanInst,
        VkPhysicalDevice *pPhysicalDevice, InstanceProperties *pInstProps)
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = pAppName;
        appInfo.applicationVersion = 1;
        appInfo.pEngineName = pEngineName;
        appInfo.engineVersion = 1;
        appInfo.apiVersion = VK_API_VERSION_1_1;
        VkInstance instance = CreateInstance(appInfo, pInstProps);

        // Enumerate Physical Device
        uint32_t physicalDeviceCount = 1;
        uint32_t const reqCount = physicalDeviceCount;
        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
        assert(physicalDeviceCount > 0 && "No GPU Found.");

        std::vector<VkPhysicalDevice> physicalDevices;
        physicalDevices.resize(physicalDeviceCount);
        VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));
        assert(physicalDeviceCount >= reqCount && "Unable to enumerate physical devices.");

        *pPhysicalDevice = SelectPhysicalDevice(physicalDevices);
        *pVulkanInst = instance;

        return true;
    }

    VkInstance CreateInstance(VkApplicationInfo appInfo, InstanceProperties *pInstProps)
    {
        VkInstance instance;

        // prepare existing extensions and layer names into a buffer for vkCreateInstance
        std::vector<const char *> instanceLayerNames;
        std::vector<const char *> instanceExtensionNames;
        pInstProps->GetExtensionNamesAndConfigs(&instanceLayerNames, &instanceExtensionNames);

        // do create the instance
        VkInstanceCreateInfo instanceCI{};
        instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCI.pNext = pInstProps->GetNext();
        instanceCI.flags = 0;
        instanceCI.pApplicationInfo = &appInfo;
        instanceCI.enabledLayerCount = (uint32_t)instanceLayerNames.size();
        instanceCI.ppEnabledLayerNames = (uint32_t)instanceLayerNames.size() ? instanceLayerNames.data() : nullptr;
        instanceCI.enabledExtensionCount = (uint32_t)instanceExtensionNames.size();
        instanceCI.ppEnabledExtensionNames = instanceExtensionNames.data();
        VK_CHECK_RESULT(vkCreateInstance(&instanceCI, nullptr, &instance));

        // Init the extensions (if they have been enabled successfuly)
        ExtDebugReportGetProcAddress(instance);
        ExtDebugReportOnCreate(instance);

        return instance;
    }

    void DestroyInstance(VkInstance instance)
    {
        vkDestroyInstance(instance, nullptr);
    }
}

