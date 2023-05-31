#include "DevicePropertiesVK.h"
#include "HelperVK.h"
#include "Misc.h"

using namespace LeoVultana_VK;

void DeviceProperties::Init(VkPhysicalDevice physicalDevice)
{
    mPhysicalDevice = physicalDevice;
    // 枚举设备扩展
    uint32_t extensionsCount = 0;
    VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, nullptr))
    mDeviceExtProperties.resize(extensionsCount);
    VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, mDeviceExtProperties.data()));
}

bool DeviceProperties::AddDeviceExtensionName(const char *deviceExtensionName)
{
    if (IsExtensionPresent(deviceExtensionName))
    {
        mDeviceExtNames.emplace_back(deviceExtensionName);
        return true;
    }

    Trace("The device extension '%s' has not been found", deviceExtensionName);

    return false;
}

void DeviceProperties::GetExtensionNamesAndConfigs(std::vector<const char *> *pDeviceExtName)
{
    for (auto &name : mDeviceExtNames) pDeviceExtName->push_back(name);
}

bool DeviceProperties::IsExtensionPresent(const char *pExtName)
{
    return std::find_if(
        mDeviceExtProperties.begin(),
        mDeviceExtProperties.end(),
        [pExtName](const VkExtensionProperties& extensionProps) -> bool {
            return strcmp(extensionProps.extensionName, pExtName) == 0;
        }) != mDeviceExtProperties.end();
}
