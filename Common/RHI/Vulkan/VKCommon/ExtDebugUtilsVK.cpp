#include "PCHVK.h"
#include "ExtDebugUtilsVK.h"

namespace LeoVultana_VK
{
    static PFN_vkSetDebugUtilsObjectNameEXT     s_vkSetDebugUtilsObjectName{};
    static PFN_vkCmdBeginDebugUtilsLabelEXT     s_vkCmdBeginDebugUtilsLabel{};
    static PFN_vkCmdEndDebugUtilsLabelEXT       s_vkCmdEndDebugUtilsLabel{};
    static bool s_bCanUseDebugUtils = false;
    static std::mutex s_mutex;

    void ExtDebugUtilsGetProAddresses(VkDevice device)
    {
        if (s_bCanUseDebugUtils)
        {
            s_vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
            s_vkCmdBeginDebugUtilsLabel = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
            s_vkCmdEndDebugUtilsLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
        }
    }

    bool ExtDebugUtilsCheckInstanceExtensions(InstanceProperties *pInstanceProp)
    {
        s_bCanUseDebugUtils = pInstanceProp->AddInstanceExtensionName("VK_EXT_debug_utils");
        return s_bCanUseDebugUtils;
    }

    void SetResourceName(VkDevice device, VkObjectType objectType, uint64_t handle, const char *name)
    {
        if (s_vkSetDebugUtilsObjectName && handle && name)
        {
            std::unique_lock<std::mutex> lock(s_mutex);

            VkDebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.objectHandle = handle;
            nameInfo.pObjectName = name;
            s_vkSetDebugUtilsObjectName(device, &nameInfo);
        }
    }

    void SetPerfMarkerBegin(VkCommandBuffer cmdBuffer, const char *name)
    {
        if (s_vkCmdBeginDebugUtilsLabel)
        {
            VkDebugUtilsLabelEXT label{};
            label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            label.pLabelName = name;
            const float color[] = {1.0f, 0.0f, 0.0f, 1.0f};
            memcpy(label.color, color, sizeof(color));
            s_vkCmdBeginDebugUtilsLabel(cmdBuffer, &label);
        }
    }

    void SetPerfMarkerEnd(VkCommandBuffer cmdBuffer)
    {
        if (s_vkCmdEndDebugUtilsLabel) s_vkCmdEndDebugUtilsLabel(cmdBuffer);
    }
}


