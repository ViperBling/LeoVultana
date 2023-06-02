#include "PCHVK.h"

#include "ExtFP16VK.h"
#include "Misc.h"

#include "Vulkan/vulkan.h"
#include "Vulkan/vulkan_win32.h"

namespace LeoVultana_VK
{
    static VkPhysicalDeviceFloat16Int8FeaturesKHR FP16Features{};
    static VkPhysicalDevice16BitStorageFeatures Storage16BitFeatures{};

    bool ExtFP16CheckExtensions(DeviceProperties *pDeviceProp)
    {
        std::vector<const char *> required_extension_names = { VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME };

        bool bFP16Enabled = true;
        for (auto& ext : required_extension_names)
        {
            if (!pDeviceProp->AddDeviceExtensionName(ext))
            {
                Trace(format("FP16 disabled, missing extension: %s\n", ext));
                bFP16Enabled = false;
            }
        }

        if (bFP16Enabled)
        {
            // Query 16 bit storage
            Storage16BitFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;

            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &Storage16BitFeatures;
            vkGetPhysicalDeviceFeatures2(pDeviceProp->GetPhysicalDevice(), &features);

            bFP16Enabled = bFP16Enabled && Storage16BitFeatures.storageBuffer16BitAccess;

            // Query 16 bit ops
            FP16Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;

            features.pNext = &FP16Features;
            vkGetPhysicalDeviceFeatures2(pDeviceProp->GetPhysicalDevice(), &features);

            bFP16Enabled = bFP16Enabled && FP16Features.shaderFloat16;
        }

        if (bFP16Enabled)
        {
            // Query 16 bit storage
            Storage16BitFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
            Storage16BitFeatures.pNext = pDeviceProp->GetNext();

            // Query 16 bit ops
            FP16Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
            FP16Features.pNext = &Storage16BitFeatures;

            pDeviceProp->SetNewNext(&FP16Features);
        }

        return bFP16Enabled;
    }
}