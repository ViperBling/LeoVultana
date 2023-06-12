#include "PCHVK.h"
#include "FreeSyncHDRVK.h"
#include "ExtFreeSyncHDRVK.h"
#include "Misc.h"
#include "HelperVK.h"
#include "Error.h"
#include "Vulkan/vulkan.h"

#include <unordered_map>

namespace LeoVultana_VK
{
    static VkSurfaceFullScreenExclusiveWin32InfoEXT s_SurfaceFullScreenExclusiveWin32InfoEXT;
    static VkSurfaceFullScreenExclusiveInfoEXT s_SurfaceFullScreenExclusiveInfoEXT;
    static VkPhysicalDeviceSurfaceInfo2KHR s_PhysicalDeviceSurfaceInfo2KHR;
    static VkDisplayNativeHdrSurfaceCapabilitiesAMD s_DisplayNativeHDRSurfaceCapabilitiesAMD;

    static VkHdrMetadataEXT s_HDRMetadataEXT;
    static VkSurfaceCapabilities2KHR s_SurfaceCapabilities2KHR;
    static VkSwapchainDisplayNativeHdrCreateInfoAMD s_SwapChainDisplayNativeHDRCreateInfoAMD;

    static VkDevice s_Device;
    static VkSurfaceKHR s_Surface;
    static VkPhysicalDevice s_PhysicalDevice;
    static HWND s_hWnd = nullptr;

    static std::unordered_map<DisplayMode, VkSurfaceFormatKHR> availableDisplayModeSurfaceFormats;

    bool s_WindowsHDRToggle = false;

    void SetHDRStructures(VkSurfaceKHR surface)
    {
        s_PhysicalDeviceSurfaceInfo2KHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
        s_PhysicalDeviceSurfaceInfo2KHR.pNext = nullptr;
        s_PhysicalDeviceSurfaceInfo2KHR.surface = surface;

        s_HDRMetadataEXT.sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
        s_HDRMetadataEXT.pNext = nullptr;
    }

    void SetFSEStructures(HWND hWnd, PresentationMode fullScreenMode)
    {
        // Required final chaining order
        // VkPhysicalDeviceSurfaceInfo2KHR -> VkSurfaceFullScreenExclusiveInfoEXT -> VkSurfaceFullScreenExclusiveWin32InfoEXT

        s_PhysicalDeviceSurfaceInfo2KHR.pNext = &s_SurfaceFullScreenExclusiveInfoEXT;
        s_SurfaceFullScreenExclusiveInfoEXT.pNext = &s_SurfaceFullScreenExclusiveWin32InfoEXT;

        s_SurfaceFullScreenExclusiveInfoEXT.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
        if (fullScreenMode == PRESENTATIONMODE_WINDOWED)
            s_SurfaceFullScreenExclusiveInfoEXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT;
        else if (fullScreenMode == PRESENTATIONMODE_BORDERLESS_FULLSCREEN)
            s_SurfaceFullScreenExclusiveInfoEXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;
        else if (fullScreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN)
            s_SurfaceFullScreenExclusiveInfoEXT.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;

        s_SurfaceFullScreenExclusiveWin32InfoEXT.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
        s_SurfaceFullScreenExclusiveWin32InfoEXT.pNext = nullptr;
        s_SurfaceFullScreenExclusiveWin32InfoEXT.hmonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    }

    void SetFreeSyncHDRStructures()
    {
        // Required final chaning order
        // VkSurfaceCapabilities2KHR -> VkDisplayNativeHdrSurfaceCapabilitiesAMD -> VkHdrMetadataEXT

        s_SurfaceCapabilities2KHR.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
        s_SurfaceCapabilities2KHR.pNext = &s_DisplayNativeHDRSurfaceCapabilitiesAMD;

        // This will cause validation error and complain as invalid structure attached.
        // But we are hijacking hdr metadata struct and attaching it here to fill it with monitor primaries
        // When getting surface capabilities
        s_DisplayNativeHDRSurfaceCapabilitiesAMD.sType = VK_STRUCTURE_TYPE_DISPLAY_NATIVE_HDR_SURFACE_CAPABILITIES_AMD;
        s_DisplayNativeHDRSurfaceCapabilitiesAMD.pNext = &s_HDRMetadataEXT;
    }

    void SetSwapChainFreeSyncHDRStructures(bool enableLocalDimming)
    {
        // Required final chaning order
        // VkSurfaceFullScreenExclusiveInfoEXT -> VkSurfaceFullScreenExclusiveWin32InfoEXT -> VkSwapchainDisplayNativeHdrCreateInfoAMD
        s_SurfaceFullScreenExclusiveWin32InfoEXT.pNext = &s_SwapChainDisplayNativeHDRCreateInfoAMD;

        s_SwapChainDisplayNativeHDRCreateInfoAMD.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_DISPLAY_NATIVE_HDR_CREATE_INFO_AMD;
        s_SwapChainDisplayNativeHDRCreateInfoAMD.pNext = nullptr;
        // Enable local dimming if supported
        // Must requry FS HDR display capabilities
        // After value is set through swapchain creation when attached to swapchain pnext.
        s_SwapChainDisplayNativeHDRCreateInfoAMD.localDimmingEnable = s_DisplayNativeHDRSurfaceCapabilitiesAMD.localDimmingSupport && enableLocalDimming;
    }

    void GetSurfaceFormats(uint32_t* pFormatCount, std::vector<VkSurfaceFormat2KHR>* surfaceFormats)
    {
        // Get list of formats
        VK_CHECK_RESULT(g_vkGetPhysicalDeviceSurfaceFormats2KHR(s_PhysicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, pFormatCount, nullptr))

        uint32_t formatCount = *pFormatCount;
        surfaceFormats->resize(formatCount);
        for (UINT i = 0; i < formatCount; ++i)
        {
            (*surfaceFormats)[i].sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        }

        VK_CHECK_RESULT(g_vkGetPhysicalDeviceSurfaceFormats2KHR(
            s_PhysicalDevice,
            &s_PhysicalDeviceSurfaceInfo2KHR,
            &formatCount, (*surfaceFormats).data()));
    }


    bool FSHDRInit(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, HWND hWnd)
    {
        s_hWnd = hWnd;
        s_Device = device;
        s_Surface = surface;
        s_PhysicalDevice = physicalDevice;

        return ExtAreFreeSyncHDRExtensionsPresent();
    }

    bool FSHDREnumerateDisplayModes(
        std::vector<DisplayMode> *pModes, bool includeFreeSyncHDR,
        PresentationMode fullscreenMode, bool enableLocalDimming)
    {
        pModes->clear();
        availableDisplayModeSurfaceFormats.clear();

        VkSurfaceFormatKHR surfaceFormat;
        surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
        surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        pModes->push_back(DISPLAYMODE_SDR);
        availableDisplayModeSurfaceFormats[DISPLAYMODE_SDR] = surfaceFormat;

        if (ExtAreHDRExtensionsPresent())
        {
            SetHDRStructures(s_Surface);

            uint32_t formatCount;
            std::vector<VkSurfaceFormat2KHR> surfFormats;
            GetSurfaceFormats(&formatCount, &surfFormats);

            for (uint32_t i = 0; i < formatCount; i++)
            {
                if ((surfFormats[i].surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                     surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
                    ||
                    (surfFormats[i].surfaceFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
                     surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT))
                {
                    // If surface formats have HDR10 format even before fullscreen surface is attached, it can only mean windows hdr toggle is on
                    s_WindowsHDRToggle = true;
                    break;
                }
            }
        } 
        else
        {
            s_PhysicalDeviceSurfaceInfo2KHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
            s_PhysicalDeviceSurfaceInfo2KHR.pNext = nullptr;
            s_PhysicalDeviceSurfaceInfo2KHR.surface = s_Surface;
        }

        if (ExtAreFSEExtensionsPresent())
        {
            SetFSEStructures(s_hWnd, fullscreenMode);
        }

        if (includeFreeSyncHDR && ExtAreFreeSyncHDRExtensionsPresent())
        {
            SetFreeSyncHDRStructures();

            // Calling get capabilities here to query for local dimming support
            FSHDRGetPhysicalDeviceSurfaceCapabilities2KHR(s_PhysicalDevice, s_Surface, nullptr);

            SetSwapChainFreeSyncHDRStructures(enableLocalDimming);
        }

        uint32_t formatCount;
        std::vector<VkSurfaceFormat2KHR> surfFormats;
        GetSurfaceFormats(&formatCount, &surfFormats);

        for (uint32_t i = 0; i < formatCount; i++)
        {
            if (surfFormats[i].surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_DISPLAY_NATIVE_AMD)
            {
                pModes->push_back(DISPLAYMODE_FSHDR_Gamma22);
                availableDisplayModeSurfaceFormats[DISPLAYMODE_FSHDR_Gamma22] = surfFormats[i].surfaceFormat;
            }
            else if (surfFormats[i].surfaceFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT &&
                     surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_DISPLAY_NATIVE_AMD)
            {
                pModes->push_back(DISPLAYMODE_FSHDR_SCRGB);
                availableDisplayModeSurfaceFormats[DISPLAYMODE_FSHDR_SCRGB] = surfFormats[i].surfaceFormat;
            }
        }

        for (uint32_t i = 0; i < formatCount; i++)
        {
            if ((surfFormats[i].surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                 surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
                 ||
                (surfFormats[i].surfaceFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
                 surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT))
            {
                if (availableDisplayModeSurfaceFormats.find(DISPLAYMODE_HDR10_2084) == availableDisplayModeSurfaceFormats.end())
                {
                    pModes->push_back(DISPLAYMODE_HDR10_2084);
                    availableDisplayModeSurfaceFormats[DISPLAYMODE_HDR10_2084] = surfFormats[i].surfaceFormat;
                }
            }
            else if (surfFormats[i].surfaceFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT &&
                     surfFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)
            {
                pModes->push_back(DISPLAYMODE_HDR10_SCRGB);
                availableDisplayModeSurfaceFormats[DISPLAYMODE_HDR10_SCRGB] = surfFormats[i].surfaceFormat;
            }
        }

        return true;
    }

    VkSurfaceFormatKHR FSHDRGetFormat(DisplayMode displayMode)
    {
        VkSurfaceFormatKHR surfaceFormat;
        surfaceFormat = availableDisplayModeSurfaceFormats[displayMode];
        return surfaceFormat;
    }

    bool FSHDRSetDisplayMode(DisplayMode displayMode, VkSwapchainKHR swapChain)
    {
        if (!ExtAreHDRExtensionsPresent())
            return false;

        switch (displayMode)
        {
            case DISPLAYMODE_SDR:
                // rec 709 primaries
                s_HDRMetadataEXT.displayPrimaryRed.x = 0.64f;
                s_HDRMetadataEXT.displayPrimaryRed.y = 0.33f;
                s_HDRMetadataEXT.displayPrimaryGreen.x = 0.30f;
                s_HDRMetadataEXT.displayPrimaryGreen.y = 0.60f;
                s_HDRMetadataEXT.displayPrimaryBlue.x = 0.15f;
                s_HDRMetadataEXT.displayPrimaryBlue.y = 0.06f;
                s_HDRMetadataEXT.whitePoint.x = 0.3127f;
                s_HDRMetadataEXT.whitePoint.y = 0.3290f;
                s_HDRMetadataEXT.minLuminance = 0.0f;
                s_HDRMetadataEXT.maxLuminance = 300.0f;
                break;
            case DISPLAYMODE_FSHDR_Gamma22:
                // use the values that we queried using VkGetPhysicalDeviceSurfaceCapabilities2KHR
                break;
            case DISPLAYMODE_FSHDR_SCRGB:
                // use the values that we queried using VkGetPhysicalDeviceSurfaceCapabilities2KHR
                break;
            case DISPLAYMODE_HDR10_2084:
                // rec 2020 primaries
                s_HDRMetadataEXT.displayPrimaryRed.x = 0.708f;
                s_HDRMetadataEXT.displayPrimaryRed.y = 0.292f;
                s_HDRMetadataEXT.displayPrimaryGreen.x = 0.170f;
                s_HDRMetadataEXT.displayPrimaryGreen.y = 0.797f;
                s_HDRMetadataEXT.displayPrimaryBlue.x = 0.131f;
                s_HDRMetadataEXT.displayPrimaryBlue.y = 0.046f;
                s_HDRMetadataEXT.whitePoint.x = 0.3127f;
                s_HDRMetadataEXT.whitePoint.y = 0.3290f;
                s_HDRMetadataEXT.minLuminance = 0.0f;
                s_HDRMetadataEXT.maxLuminance = 1000.0f; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
                s_HDRMetadataEXT.maxContentLightLevel = 1000.0f;
                s_HDRMetadataEXT.maxFrameAverageLightLevel = 400.0f; // max and average content light level data will be used to do tonemapping on display
                break;
            case DISPLAYMODE_HDR10_SCRGB:
                // rec 709 primaries
                s_HDRMetadataEXT.displayPrimaryRed.x = 0.64f;
                s_HDRMetadataEXT.displayPrimaryRed.y = 0.33f;
                s_HDRMetadataEXT.displayPrimaryGreen.x = 0.30f;
                s_HDRMetadataEXT.displayPrimaryGreen.y = 0.60f;
                s_HDRMetadataEXT.displayPrimaryBlue.x = 0.15f;
                s_HDRMetadataEXT.displayPrimaryBlue.y = 0.06f;
                s_HDRMetadataEXT.whitePoint.x = 0.3127f;
                s_HDRMetadataEXT.whitePoint.y = 0.3290f;
                s_HDRMetadataEXT.minLuminance = 0.0f;
                s_HDRMetadataEXT.maxLuminance = 1000.0f; // This will cause tonemapping to happen on display end as long as it's greater than display's actual queried max luminance. The look will change and it will be display dependent!
                s_HDRMetadataEXT.maxContentLightLevel = 1000.0f;
                s_HDRMetadataEXT.maxFrameAverageLightLevel = 400.0f; // max and average content light level data will be used to do tonemapping on display
                break;
        }

        g_vkSetHdrMetadataEXT(s_Device, 1, &swapChain, &s_HDRMetadataEXT);

        return true;
    }

    const char *FSHDRGetDisplayModeString(DisplayMode displayMode)
    {
        // note that these string must match the order of the DisplayModes enum
        const char *DisplayModesStrings[]
        {
            "SDR",
            "FSHDR_Gamma22",
            "FSHDR_SCRGB",
            "HDR10_2084",
            "HDR10_SCRGB"
        };
        return DisplayModesStrings[displayMode];
    }

    const VkHdrMetadataEXT *FSHDRGetDisplayInfo()
    {
        return &s_HDRMetadataEXT;
    }

    void FSHDRSetLocalDimmingMode(VkSwapchainKHR swapChain, VkBool32 localDimmingEnable)
    {
        g_vkSetLocalDimmingAMD(s_Device, swapChain, localDimmingEnable);
        VK_CHECK_RESULT(g_vkGetPhysicalDeviceSurfaceCapabilities2KHR(s_PhysicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, &s_SurfaceCapabilities2KHR));
        g_vkSetHdrMetadataEXT(s_Device, 1, &swapChain, &s_HDRMetadataEXT);
    }

    void FSHDRSetFullscreenState(bool fullscreen, VkSwapchainKHR swapChain)
    {
        if (fullscreen)
            VK_CHECK_RESULT(g_vkAcquireFullScreenExclusiveModeEXT(s_Device, swapChain))
        else
            VK_CHECK_RESULT(g_vkReleaseFullScreenExclusiveModeEXT(s_Device, swapChain));
    }

    void FSHDRGetPhysicalDeviceSurfaceCapabilities2KHR(
        VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
        VkSurfaceCapabilitiesKHR *pSurfCapabilities)
    {
        assert(surface == s_Surface);
        assert(physicalDevice == s_PhysicalDevice);

        VK_CHECK_RESULT(g_vkGetPhysicalDeviceSurfaceCapabilities2KHR(
            s_PhysicalDevice, &s_PhysicalDeviceSurfaceInfo2KHR, &s_SurfaceCapabilities2KHR));

        if (pSurfCapabilities)
            *pSurfCapabilities = s_SurfaceCapabilities2KHR.surfaceCapabilities;
    }

    VkSurfaceFullScreenExclusiveInfoEXT *GetVkSurfaceFullScreenExclusiveInfoEXT()
    {
        return &s_SurfaceFullScreenExclusiveInfoEXT;
    }

    // We are turning off this HDR path for now
    // Driver update to get this path working is coming soon.
#define WINDOW_HDR_PATH 0
    bool CheckIfWindowModeHDROn()
    {
#if WINDOW_HDR_PATH
        return s_WindowsHDRToggle;
#else
        return false;
#endif
    }
}
