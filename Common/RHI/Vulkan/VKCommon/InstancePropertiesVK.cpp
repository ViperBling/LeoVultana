#include "InstancePropertiesVK.h"
#include "HelperVK.h"
#include "Misc.h"

using namespace LeoVultana_VK;

void InstanceProperties::Init()
{
    // 查询Instance Layer
    uint32_t instLayerPropCount = 0;
    VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instLayerPropCount, nullptr));
    mInstanceLayerProperties.resize(instLayerPropCount);
    if (instLayerPropCount > 0)
    {
        VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instLayerPropCount, mInstanceLayerProperties.data()));
    }
    // 查询Instance Extension
    uint32_t instExtPropCount = 0;
    VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &instExtPropCount, nullptr));
    mInstanceExtensionProperties.resize(instExtPropCount);
    if (instExtPropCount > 0)
    {
        VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &instExtPropCount, mInstanceExtensionProperties.data()));
    }
}

bool InstanceProperties::AddInstanceLayerName(const char *instLayerName)
{
    if (IsLayerPresent(instLayerName))
    {
        mInstanceLayerNames.push_back(instLayerName);
        return true;
    }
    Trace("The instance layer '%s' has not been found!\n", instLayerName);
    return false;
}

bool InstanceProperties::AddInstanceExtensionName(const char *instExtName)
{
    if (IsExtensionPresent(instExtName))
    {
        mInstanceExtensionNames.push_back(instExtName);
        return true;
    }
    Trace("The instance extension '%s' has not been found!\n", instExtName);
    return false;
}

void InstanceProperties::GetExtensionNamesAndConfigs(
    std::vector<const char *> *pInstLayerName,
    std::vector<const char *> *pInstExtName)
{
    for (auto & name : mInstanceLayerNames) pInstLayerName->emplace_back(name);
    for (auto & name : mInstanceExtensionNames) pInstExtName->emplace_back(name);
}

bool InstanceProperties::IsLayerPresent(const char *pLayerName)
{
    return std::find_if(
        mInstanceLayerProperties.begin(),
        mInstanceLayerProperties.end(),
        [pLayerName](const VkLayerProperties& layerProps)->bool {
            return strcmp(layerProps.layerName, pLayerName) == 0;
        }) != mInstanceLayerProperties.end();
}

bool InstanceProperties::IsExtensionPresent(const char *pExtensionName)
{
    return std::find_if(
        mInstanceExtensionProperties.begin(),
        mInstanceExtensionProperties.end(),
        [pExtensionName](const VkExtensionProperties & extensionProps)->bool {
            return strcmp(extensionProps.extensionName, pExtensionName) == 0;
        }) != mInstanceExtensionProperties.end();
}
