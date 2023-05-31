#include "ExtFP16VK.h"
#include "Vulkan/vulkan.h"
#include "Vulkan/vulkan_win32.h"

namespace LeoVultana_VK
{
    static VkPhysicalDeviceRayTracingPipelineFeaturesKHR RTPipelinesFeatures{};
    static VkPhysicalDeviceRayQueryFeaturesKHR RTQueryFeatures{};
    static VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures{};
    static VkPhysicalDeviceBufferDeviceAddressFeatures BufferDeviceAddressFeatures{};
    static VkPhysicalDeviceDescriptorIndexingFeatures DescriptorIndexingFeatures{};

    void ExtRTCheckExtensions(DeviceProperties* pDP, bool& RT10Supported, bool& RT11Supported)
    {
        bool RT10ExtensionPresent = pDP->AddDeviceExtensionName(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        bool RT11ExtensionPresent = pDP->AddDeviceExtensionName(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        bool AccelerationStructureExtensionPresent = pDP->AddDeviceExtensionName(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        bool BufferDeviceAddressPresent = pDP->AddDeviceExtensionName(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        bool DescriptorIndexingExtensionPresent = pDP->AddDeviceExtensionName(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        pDP->AddDeviceExtensionName(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        pDP->AddDeviceExtensionName(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
        pDP->AddDeviceExtensionName(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

        if (RT10ExtensionPresent)
        {
            RTPipelinesFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &RTPipelinesFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);

            RT10Supported = RTPipelinesFeatures.rayTracingPipeline;

            if (RT10Supported)
            {
                RTPipelinesFeatures.pNext = pDP->GetNext();
                pDP->SetNewNext(&RTPipelinesFeatures);
            }
        }

        if (RT11ExtensionPresent)
        {
            RTQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &RTQueryFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);

            RT11Supported = RTQueryFeatures.rayQuery;

            if (RT11Supported)
            {
                RTQueryFeatures.pNext = pDP->GetNext();
                pDP->SetNewNext(&RTQueryFeatures);
            }
        }

        if (AccelerationStructureExtensionPresent)
        {
            AccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &AccelerationStructureFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);

            if (AccelerationStructureFeatures.accelerationStructure == VK_TRUE)
            {
                // we already have 2 feature structures chained, so we aren't setting pNext to the same structure that will become the Next one
                AccelerationStructureFeatures.pNext = pDP->GetNext();
                pDP->SetNewNext(&AccelerationStructureFeatures);
            }
        }

        if (DescriptorIndexingExtensionPresent)
        {
            DescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &DescriptorIndexingFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);

            DescriptorIndexingFeatures.pNext = pDP->GetNext();
            pDP->SetNewNext(&DescriptorIndexingFeatures);
        }

        if (BufferDeviceAddressPresent)
        {
            BufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &BufferDeviceAddressFeatures;
            vkGetPhysicalDeviceFeatures2(pDP->GetPhysicalDevice(), &features);

            if (BufferDeviceAddressFeatures.bufferDeviceAddress == VK_TRUE)
            {
                BufferDeviceAddressFeatures.pNext = pDP->GetNext();
                pDP->SetNewNext(&BufferDeviceAddressFeatures);
            }
        }
    }
}