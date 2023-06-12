#include "ExtValidationVK.h"
#include "InstanceVK.h"
#include "InstancePropertiesVK.h"
#include "DevicePropertiesVK.h"
#include "HelperVK.h"

namespace LeoVultana_VK
{
    const char instanceExtensionName[] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    const char instanceLayerName[] = "VK_LAYER_KHRONOS_validation";
    const VkValidationFeatureEnableEXT featuresRequested[] = {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
    VkValidationFeaturesEXT features = {};

    static PFN_vkCreateDebugReportCallbackEXT g_vkCreateDebugReportCallbackEXT{};
    static PFN_vkDebugReportMessageEXT g_vkDebugReportMessageEXT{};
    static PFN_vkDestroyDebugReportCallbackEXT g_vkDestroyDebugReportCallbackEXT{};
    static VkDebugReportCallbackEXT g_DebugReportCallback{};

    static bool s_bCanUseDebugReport = false;

    static VKAPI_ATTR VkBool32 VKAPI_CALL CustomDebugReportCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t object,
        size_t location,
        int32_t messageCode,
        const char* pLayerPrefix,
        const char* pMessage,
        void* pUserData)
    {
        OutputDebugStringA(pMessage);
        OutputDebugStringA("\n");
        return VK_FALSE;
    }

    bool ExtDebugReportCheckInstanceExtensions(InstanceProperties *pInstProps, bool gpuValidation)
    {
        s_bCanUseDebugReport = pInstProps->AddInstanceLayerName(instanceLayerName) && pInstProps->AddInstanceExtensionName(instanceExtensionName);
        if (s_bCanUseDebugReport && gpuValidation)
        {
            features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            features.pNext = pInstProps->GetNext();
            features.enabledValidationFeatureCount = _countof( featuresRequested );
            features.pEnabledValidationFeatures = featuresRequested;

            pInstProps->SetNewNext(&features);
        }

        return s_bCanUseDebugReport;
    }

    void ExtDebugReportGetProcAddress(VkInstance instance)
    {
        if (s_bCanUseDebugReport)
        {
            g_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
            g_vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");
            g_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
            assert(g_vkCreateDebugReportCallbackEXT);
            assert(g_vkDebugReportMessageEXT);
            assert(g_vkDestroyDebugReportCallbackEXT);
        }
    }

    void ExtDebugReportOnCreate(VkInstance instance)
    {
        if (g_vkCreateDebugReportCallbackEXT)
        {
            VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
            debugReportCallbackInfo.flags = 
                VK_DEBUG_REPORT_ERROR_BIT_EXT 
                | VK_DEBUG_REPORT_WARNING_BIT_EXT 
                | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            debugReportCallbackInfo.pfnCallback = CustomDebugReportCallback;
            VK_CHECK_RESULT(g_vkCreateDebugReportCallbackEXT(instance, &debugReportCallbackInfo, nullptr, &g_DebugReportCallback))
        }
    }

    void ExtDebugReportOnDestroy(VkInstance instance)
    {
        // It should happen after destroing device, before destroying instance.
        if (g_DebugReportCallback)
        {
            g_vkDestroyDebugReportCallbackEXT(instance, g_DebugReportCallback, nullptr);
            g_DebugReportCallback = nullptr;
        }
    }
}

