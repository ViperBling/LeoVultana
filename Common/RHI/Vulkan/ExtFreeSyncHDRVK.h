#pragma once

#include "DevicePropertiesVK.h"
#include "InstancePropertiesVK.h"

#include <Vulkan/vulkan.h>
#include <Vulkan/vulkan_win32.h>

namespace LeoVultana_VK
{
    extern PFN_vkGetDeviceProcAddr                        g_vkGetDeviceProcAddr;

    // Functions for regular HDR ex: HDR10
    extern PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR g_vkGetPhysicalDeviceSurfaceCapabilities2KHR;
    extern PFN_vkGetPhysicalDeviceSurfaceFormats2KHR      g_vkGetPhysicalDeviceSurfaceFormats2KHR;
    extern PFN_vkSetHdrMetadataEXT                        g_vkSetHdrMetadataEXT;

    // Functions for FSE required if trying to use Freesync HDR
    extern PFN_vkAcquireFullScreenExclusiveModeEXT        g_vkAcquireFullScreenExclusiveModeEXT;
    extern PFN_vkReleaseFullScreenExclusiveModeEXT        g_vkReleaseFullScreenExclusiveModeEXT;

    // Functions for Freesync HDR
    extern PFN_vkSetLocalDimmingAMD                       g_vkSetLocalDimmingAMD;

    void ExtCheckHDRInstanceExtensions(InstanceProperties *pIP);
    void ExtCheckHDRDeviceExtensions(DeviceProperties *pDP);
    void ExtCheckFSEDeviceExtensions(DeviceProperties *pDP);
    void ExtCheckFreeSyncHDRDeviceExtensions(DeviceProperties *pDP);

    void ExtGetHDRFSEFreesyncHDRProcAddresses(VkInstance instance, VkDevice device);

    bool ExtAreHDRExtensionsPresent();
    bool ExtAreFSEExtensionsPresent();
    bool ExtAreFreeSyncHDRExtensionsPresent();
}