#include "DeviceVK.h"
#include "InstanceVK.h"
#include "ExtDebugUtilsVK.h"
#include "ExtFreeSyncHDRVK.h"
#include "ExtFP16VK.h"
#include "ExtRayTracingVK.h"
#include "ExtVRSVK.h"
#include "ExtValidationVK.h"
#include "Misc.h"

#include <Vulkan/vulkan_win32.h>

#ifdef USE_VMA
#define VMA_IMPLEMENTATION
#include <vulkan/vk_mem_alloc.h>
#endif


using namespace LeoVultana_VK;

Device::Device()
{

}

Device::~Device()
{

}

void Device::OnCreate(
    const char *pAppName, const char *pEngineName,
    bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, HWND hWnd)
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
    bool gpuValidationLayerEnabled, InstanceProperties *pInstProp)
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
    ExtCheckFreeSyncHDRDeviceExtensions(pDeviceProp);
    pDeviceProp->AddDeviceExtensionName(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    pDeviceProp->AddDeviceExtensionName(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
}

void Device::OnCreateEx(
    VkInstance vulkanInstance,
    VkPhysicalDevice physicalDevice,
    HWND hWnd, DeviceProperties *pDeviceProp)
{
    VkResult res;
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

#if defined(_WIN32)
    // Create a Win32 Surface
    VkWin32SurfaceCreateInfoKHR win32SurfaceCI{};
    win32SurfaceCI.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    win32SurfaceCI.pNext = nullptr;
    win32SurfaceCI.hinstance = nullptr;
    win32SurfaceCI.hwnd = hWnd;
    res = vkCreateWin32SurfaceKHR(mInstance, &win32SurfaceCI, nullptr, &mSurface);
#else
    #error platform not supported
#endif

    // Find GPU
    mGraphicsQueueFamilyIndex = UINT32_MAX;
    mPresentQueueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (mGraphicsQueueFamilyIndex == UINT32_MAX) mGraphicsQueueFamilyIndex = i;

            
        }
    }
}

void Device::OnDestroy() {

}

void Device::GetDeviceInfo(std::string *deviceName, std::string *driverVersion) {

}

void Device::CreatePipelineCache() {

}

void Device::DestroyPipelineCache() {

}

VkPipelineCache Device::GetPipelineCache() {
    return nullptr;
}

void Device::GPUFlush() {

}
