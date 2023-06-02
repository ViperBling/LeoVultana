#include "PCHVK.h"

#include "ExtFreeSyncHDRVK.h"
#include "Misc.h"
#include "Error.h"

namespace LeoVultana_VK
{
    extern PFN_vkGetDeviceProcAddr                        g_vkGetDeviceProcAddr{};

    // Functions for regular HDR ex: HDR10
    extern PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR g_vkGetPhysicalDeviceSurfaceCapabilities2KHR{};
    extern PFN_vkGetPhysicalDeviceSurfaceFormats2KHR      g_vkGetPhysicalDeviceSurfaceFormats2KHR{};
    extern PFN_vkSetHdrMetadataEXT                        g_vkSetHdrMetadataEXT{};

    // Functions for FSE required if trying to use Freesync HDR
    extern PFN_vkAcquireFullScreenExclusiveModeEXT        g_vkAcquireFullScreenExclusiveModeEXT{};
    extern PFN_vkReleaseFullScreenExclusiveModeEXT        g_vkReleaseFullScreenExclusiveModeEXT{};

    // Functions for Freesync HDR
    extern PFN_vkSetLocalDimmingAMD                       g_vkSetLocalDimmingAMD{};

    static VkPhysicalDeviceSurfaceInfo2KHR s_PhysicalDeviceSurfaceInfo2KHR;

    static VkSurfaceFullScreenExclusiveWin32InfoEXT s_SurfaceFullScreenExclusiveWin32InfoEXT;
    static VkSurfaceFullScreenExclusiveInfoEXT s_SurfaceFullScreenExclusiveInfoEXT;

    static VkDisplayNativeHdrSurfaceCapabilitiesAMD s_DisplayNativeHdrSurfaceCapabilitiesAMD;

    static bool s_isHdrInstanceExtensionPresent = false;
    static bool s_isHdrDeviceExtensionsPresent = false;
    static bool s_isFSEDeviceExtensionsPresent = false;
    static bool s_isFSHDRDeviceExtensionsPresent = false;

    void ExtCheckHDRInstanceExtensions(InstanceProperties *pInstProp)
    {
        s_isHdrInstanceExtensionPresent = pInstProp->AddInstanceExtensionName(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    }

    void ExtCheckHDRDeviceExtensions(DeviceProperties *pDeviceProp)
    {
        s_isHdrDeviceExtensionsPresent = pDeviceProp->AddDeviceExtensionName(VK_EXT_HDR_METADATA_EXTENSION_NAME);
    }

    void ExtCheckFSEDeviceExtensions(DeviceProperties *pDeviceProp)
    {
        s_isFSEDeviceExtensionsPresent = pDeviceProp->AddDeviceExtensionName(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
    }

    void ExtCheckFreeSyncHDRDeviceExtensions(DeviceProperties *pDeviceProp)
    {
        s_isFSHDRDeviceExtensionsPresent = pDeviceProp->AddDeviceExtensionName(VK_AMD_DISPLAY_NATIVE_HDR_EXTENSION_NAME);
    }

    void ExtGetHDRFSEFreeSyncHDRProcAddresses(VkInstance instance, VkDevice device)
    {
        if (s_isHdrInstanceExtensionPresent)
        {
            g_vkGetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
            assert(g_vkGetPhysicalDeviceSurfaceCapabilities2KHR);

            g_vkGetPhysicalDeviceSurfaceFormats2KHR = (PFN_vkGetPhysicalDeviceSurfaceFormats2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormats2KHR");
            assert(g_vkGetPhysicalDeviceSurfaceFormats2KHR);
        }

        g_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
        assert(g_vkGetDeviceProcAddr);

        if (s_isHdrDeviceExtensionsPresent)
        {
            g_vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)g_vkGetDeviceProcAddr(device, "vkSetHdrMetadataEXT");
            assert(g_vkSetHdrMetadataEXT);
        }

        if (s_isFSEDeviceExtensionsPresent)
        {
            g_vkAcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT)g_vkGetDeviceProcAddr(device, "vkAcquireFullScreenExclusiveModeEXT");
            assert(g_vkAcquireFullScreenExclusiveModeEXT);

            g_vkReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT)g_vkGetDeviceProcAddr(device, "vkReleaseFullScreenExclusiveModeEXT");
            assert(g_vkReleaseFullScreenExclusiveModeEXT);
        }

        if (s_isFSHDRDeviceExtensionsPresent)
        {
            g_vkSetLocalDimmingAMD = (PFN_vkSetLocalDimmingAMD)g_vkGetDeviceProcAddr(device, "vkSetLocalDimmingAMD");
            assert(g_vkSetLocalDimmingAMD);
        }
    }

    bool ExtAreHDRExtensionsPresent()
    {
        return s_isHdrInstanceExtensionPresent && s_isHdrDeviceExtensionsPresent;
    }

    bool ExtAreFSEExtensionsPresent()
    {
        return s_isFSEDeviceExtensionsPresent;
    }

    bool ExtAreFreeSyncHDRExtensionsPresent()
    {
        return s_isHdrInstanceExtensionPresent &&
               s_isHdrDeviceExtensionsPresent &&
               s_isFSEDeviceExtensionsPresent &&
               s_isFSHDRDeviceExtensionsPresent;
    }
}
