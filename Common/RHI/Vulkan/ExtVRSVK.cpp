#include "PCHVK.h"
#include "ExtVRSVK.h"
#include "Misc.h"
#include <Vulkan/vulkan.h>
#include <Vulkan/vulkan_win32.h>

using namespace LeoVultana_VK;

namespace LeoVultana_VK
{
    static VkPhysicalDeviceFragmentShadingRateFeaturesKHR VRSQueryFeatures{};
}

void ExtVRSCheckExtensions(DeviceProperties *pDeviceProp, bool &Tier1Supported, bool &Tier2Supported)
{
    if (pDeviceProp->AddDeviceExtensionName(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
    {
        pDeviceProp->AddDeviceExtensionName(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);

        VRSQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
        VkPhysicalDeviceFeatures2 features{};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features.pNext = &VRSQueryFeatures;
        vkGetPhysicalDeviceFeatures2(pDeviceProp->GetPhysicalDevice(), &features);

        Tier1Supported = VRSQueryFeatures.pipelineFragmentShadingRate;
        Tier2Supported = VRSQueryFeatures.attachmentFragmentShadingRate && VRSQueryFeatures.primitiveFragmentShadingRate;
    }
}
