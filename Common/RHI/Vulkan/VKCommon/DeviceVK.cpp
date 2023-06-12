#include "HelperVK.h"
#include "DeviceVK.h"
#include "InstanceVK.h"
#include "ExtDebugUtilsVK.h"
#include "ExtFreeSyncHDRVK.h"
#include "ExtFP16VK.h"
#include "ExtRayTracingVK.h"
#include "ExtVRSVK.h"
#include "ExtValidationVK.h"
#include "Misc.h"

#include "Vulkan/vulkan_win32.h"

#ifdef USE_VMA
#define VMA_IMPLEMENTATION
#include "Vulkan/vk_mem_alloc.h"
#endif

using namespace LeoVultana_VK;

Device::Device()
{
}

Device::~Device()
{
}

void Device::OnCreate(
    const char *pAppName, 
    const char *pEngineName,
    bool cpuValidationLayerEnabled, 
    bool gpuValidationLayerEnabled, HWND hWnd)
{
    InstanceProperties instProp;
    instProp.Init();
    SetEssentialInstanceExtensions(cpuValidationLayerEnabled, gpuValidationLayerEnabled, &instProp);

    // Create Instance
    VkInstance vulkanInstance;
    VkPhysicalDevice physicalDevice;
    CreateInstance(pAppName, pEngineName, &vulkanInstance, &physicalDevice, &instProp);

    DeviceProperties deviceProp;
    deviceProp.Init(physicalDevice);
    SetEssentialDeviceExtensions(&deviceProp);

    // Create Device
    OnCreateEx(vulkanInstance, physicalDevice, hWnd, &deviceProp);
}

void Device::SetEssentialInstanceExtensions(
    bool cpuValidationLayerEnabled,
    bool gpuValidationLayerEnabled, 
    InstanceProperties *pInstProp)
{
    pInstProp->AddInstanceExtensionName(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    pInstProp->AddInstanceExtensionName(VK_KHR_SURFACE_EXTENSION_NAME);
    ExtCheckHDRInstanceExtensions(pInstProp);
    ExtDebugUtilsCheckInstanceExtensions(pInstProp);
    if (cpuValidationLayerEnabled) ExtDebugReportCheckInstanceExtensions(pInstProp, gpuValidationLayerEnabled);
}

void Device::SetEssentialDeviceExtensions(DeviceProperties *pDeviceProp)
{
    mUsingFP16 = ExtFP16CheckExtensions(pDeviceProp);
    ExtRTCheckExtensions(pDeviceProp, mRT10Supported, mRT11Supported);
    ExtVRSCheckExtensions(pDeviceProp, mVRS1Supported, mVRS2Supported);
    ExtCheckHDRDeviceExtensions(pDeviceProp);
    ExtCheckFSEDeviceExtensions(pDeviceProp);
    ExtCheckFreeSyncHDRDeviceExtensions(pDeviceProp);
    pDeviceProp->AddDeviceExtensionName(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    pDeviceProp->AddDeviceExtensionName(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
}

void Device::OnCreateEx(
    VkInstance vulkanInstance,
    VkPhysicalDevice physicalDevice,
    HWND hWnd, DeviceProperties *pDeviceProp)
{
    mInstance = vulkanInstance;
    mPhysicalDevice = physicalDevice;

    // Get Queue/Memory/Device Properties
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);
    assert(queueFamilyCount >= 1);

    std::vector<VkQueueFamilyProperties> queueProps;
    queueProps.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, queueProps.data());
    assert(queueFamilyCount >= 1);

    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mMemProperties);
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &mDeviceProperties);

    // Get subgroup properties to check if subgroup operations are supported
    mSubgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
    mSubgroupProperties.pNext = nullptr;

    mDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    mDeviceProperties2.pNext = &mSubgroupProperties;

    vkGetPhysicalDeviceProperties2(mPhysicalDevice, &mDeviceProperties2);

    // Create a Win32 Surface
    VkWin32SurfaceCreateInfoKHR win32SurfaceCI{};
    win32SurfaceCI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCI.pNext = nullptr;
    win32SurfaceCI.hinstance = nullptr;
    win32SurfaceCI.hwnd = hWnd;
    VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(mInstance, &win32SurfaceCI, nullptr, &mSurface));

    // Find GPU
    mGraphicsQueueFamilyIndex = UINT32_MAX;
    mPresentQueueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (mGraphicsQueueFamilyIndex == UINT32_MAX) mGraphicsQueueFamilyIndex = i;
            VkBool32 supportsPresent;
            vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, mSurface, &supportsPresent);
            if (supportsPresent == VK_TRUE)
            {
                mGraphicsQueueFamilyIndex = i;
                mPresentQueueFamilyIndex = i;
                break;
            }
        }
    }
    // If didn't find a queue that supports both graphics and present, then
    // find a separate present queue.
    if (mPresentQueueFamilyIndex == UINT32_MAX)
    {
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            VkBool32 supportPresent;
            vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, mSurface, &supportPresent);
            if (supportPresent == VK_TRUE)
            {
                mPresentQueueFamilyIndex = (uint32_t)i;
                break;
            }
        }
    }

    mComputeQueueFamilyIndex = UINT32_MAX;

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if ((queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
        {
            if (mComputeQueueFamilyIndex == UINT32_MAX) mComputeQueueFamilyIndex = i;
            if (mGraphicsQueueFamilyIndex != i)
            {
                mComputeQueueFamilyIndex = i;
                break;
            }
        }
    }

    // prepare existing extensions names into a buffer for vkCreateDevice
    std::vector<const char*> extensionNames;
    pDeviceProp->GetExtensionNamesAndConfigs(&extensionNames);

    // Create Device
    float queuePriorities[1] = {0.0};
    VkDeviceQueueCreateInfo deviceQueueCI[2]{};
    deviceQueueCI[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCI[0].pNext = nullptr;
    deviceQueueCI[0].queueCount = 1;
    deviceQueueCI[0].pQueuePriorities = queuePriorities;
    deviceQueueCI[0].queueFamilyIndex = mGraphicsQueueFamilyIndex;
    deviceQueueCI[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCI[1].pNext = nullptr;
    deviceQueueCI[1].queueCount = 1;
    deviceQueueCI[1].pQueuePriorities = queuePriorities;
    deviceQueueCI[1].queueFamilyIndex = mComputeQueueFamilyIndex;

    VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
    physicalDeviceFeatures.fillModeNonSolid = true;
    physicalDeviceFeatures.pipelineStatisticsQuery = true;
    physicalDeviceFeatures.fragmentStoresAndAtomics = true;
    physicalDeviceFeatures.vertexPipelineStoresAndAtomics = true;
    physicalDeviceFeatures.shaderImageGatherExtended = true;
    physicalDeviceFeatures.wideLines = true; //needed for drawing lines with a specific width.
    physicalDeviceFeatures.independentBlend = true; // needed for having different blend for each render target 

    // enable feature for FP16
    VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR shaderSubgroupExtendedType = {};
    shaderSubgroupExtendedType.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES_KHR;
    shaderSubgroupExtendedType.pNext = pDeviceProp->GetNext(); //used to be pNext of VkDeviceCreateInfo
    shaderSubgroupExtendedType.shaderSubgroupExtendedTypes = VK_TRUE;

    VkPhysicalDeviceRobustness2FeaturesEXT robustness2{};
    robustness2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
    robustness2.pNext = &shaderSubgroupExtendedType;
    robustness2.nullDescriptor = VK_TRUE;

    // 允许绑定Null View
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
    physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.features = physicalDeviceFeatures;
    physicalDeviceFeatures2.pNext = &robustness2;

    VkDeviceCreateInfo deviceCI{};
    deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.pNext = &physicalDeviceFeatures2;
    deviceCI.queueCreateInfoCount = 2;
    deviceCI.pQueueCreateInfos = deviceQueueCI;
    deviceCI.enabledExtensionCount = (uint32_t)extensionNames.size();
    deviceCI.ppEnabledExtensionNames = deviceCI.enabledExtensionCount ? extensionNames.data() : nullptr;
    deviceCI.pEnabledFeatures = nullptr;
    VK_CHECK_RESULT(vkCreateDevice(mPhysicalDevice, &deviceCI, nullptr, &mDevice));

#ifdef USE_VMA
    VmaAllocatorCreateInfo vmaAllocatorCI{};
    vmaAllocatorCI.physicalDevice = GetPhysicalDevice();
    vmaAllocatorCI.device = GetDevice();
    vmaAllocatorCI.instance = mInstance;
    vmaCreateAllocator(&vmaAllocatorCI, &m_hAllocator);
#endif

    // 创建队列
    vkGetDeviceQueue(mDevice, mGraphicsQueueFamilyIndex, 0, &mGraphicsQueue);
    if (mGraphicsQueueFamilyIndex == mPresentQueueFamilyIndex) mPresentQueue = mGraphicsQueue;
    else vkGetDeviceQueue(mDevice, mPresentQueueFamilyIndex, 0, &mPresentQueue);

    if (mComputeQueueFamilyIndex != UINT32_MAX)
        vkGetDeviceQueue(mDevice, mComputeQueueFamilyIndex, 0, &mComputeQueue);

    // 初始化扩展
    ExtDebugUtilsGetProAddresses(mDevice);
    ExtGetHDRFSEFreeSyncHDRProcAddresses(mInstance, mDevice);
}

void Device::OnDestroy()
{
    if (mSurface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    }
#ifdef USE_VMA
    vmaDestroyAllocator(m_hAllocator);
    m_hAllocator = nullptr;
#endif

    if (mDevice != VK_NULL_HANDLE)
    {
        vkDestroyDevice(mDevice, nullptr);
        mDevice = nullptr;
    }

    ExtDebugReportOnDestroy(mInstance);
    DestroyInstance(mInstance);
    mInstance = VK_NULL_HANDLE;
}

void Device::GetDeviceInfo(std::string *deviceName, std::string *driverVersion)
{
#define EXTRACT(v,offset, length) ((v>>offset) & ((1<<length)-1))
    *deviceName = mDeviceProperties.deviceName;
    *driverVersion = format(
        "%i.%i.%i",
        EXTRACT(mDeviceProperties.driverVersion, 22, 10),
        EXTRACT(mDeviceProperties.driverVersion, 14, 8),
        EXTRACT(mDeviceProperties.driverVersion, 0, 16));
}

void Device::CreatePipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCI{};
    pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheCI.pNext = nullptr;
    pipelineCacheCI.initialDataSize = 0;
    pipelineCacheCI.pInitialData = nullptr;
    pipelineCacheCI.flags = 0;
    VK_CHECK_RESULT(vkCreatePipelineCache(mDevice, &pipelineCacheCI, nullptr, &mPipelineCache));
}

void Device::DestroyPipelineCache()
{
    vkDestroyPipelineCache(mDevice, mPipelineCache, nullptr);
}

void Device::GPUFlush()
{
    vkDeviceWaitIdle(mDevice);
}


bool LeoVultana_VK::MemoryTypeFromProperties(VkPhysicalDeviceMemoryProperties&& memoryProp, uint32_t typeBits, VkFlags requirementsMask, uint32_t *typeIndex)
{
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < memoryProp.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memoryProp.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)
            {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

bool LeoVultana_VK::MemoryTypeFromProperties(VkPhysicalDeviceMemoryProperties& memoryProp, uint32_t typeBits, VkFlags requirementsMask, uint32_t *typeIndex)
{
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < memoryProp.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memoryProp.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)
            {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}