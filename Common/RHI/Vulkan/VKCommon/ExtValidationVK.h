#pragma once

#include "InstancePropertiesVK.h"
#include "DevicePropertiesVK.h"

namespace LeoVultana_VK
{
    bool ExtDebugReportCheckInstanceExtensions(InstanceProperties* pInstProps, bool gpuValidation);
    void ExtDebugReportGetProcAddress(VkInstance instance);

    void ExtDebugReportOnCreate(VkInstance instance);
    void ExtDebugReportOnDestroy(VkInstance instance);
}