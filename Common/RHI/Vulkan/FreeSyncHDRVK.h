#pragma once

#include "DevicePropertiesVK.h"
#include "InstancePropertiesVK.h"
#include <Vulkan/vulkan_win32.h>

namespace LeoVultana_VK
{
    enum PresentationMode
    {
        PRESENTATIONMODE_WINDOWED,
        PRESENTATIONMODE_BORDERLESS_FULLSCREEN,
        PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN
    };

    enum DisplayMode
    {
        DISPLAYMODE_SDR,
        DISPLAYMODE_FSHDR_Gamma22,
        DISPLAYMODE_FSHDR_SCRGB,
        DISPLAYMODE_HDR10_2084,
        DISPLAYMODE_HDR10_SCRGB
    };

    // only the swapchain should be using these functions

    bool FSHDRInit(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, HWND hWnd);
    bool FSHDREnumerateDisplayModes(std::vector<DisplayMode> *pModes, bool includeFreeSyncHDR, PresentationMode fullscreenMode = PRESENTATIONMODE_WINDOWED, bool enableLocalDimming = true);
    VkSurfaceFormatKHR FSHDRGetFormat(DisplayMode displayMode);
    bool FSHDRSetDisplayMode(DisplayMode displayMode, VkSwapchainKHR swapChain);
    const char* FSHDRGetDisplayModeString(DisplayMode displayMode);
    const VkHdrMetadataEXT* FSHDRGetDisplayInfo();

    void FSHDRSetLocalDimmingMode(VkSwapchainKHR swapChain, VkBool32 localDimmingEnable);
    void FSHDRSetFullscreenState(bool fullscreen, VkSwapchainKHR swapChain);

    void FSHDRGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfCapabilities);
    VkSurfaceFullScreenExclusiveInfoEXT *GetVkSurfaceFullScreenExclusiveInfoEXT();

    bool CheckIfWindowModeHDROn();
}