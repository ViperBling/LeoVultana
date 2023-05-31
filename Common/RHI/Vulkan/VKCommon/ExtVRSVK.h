#pragma once

#include "DevicePropertiesVK.h"
#include "InstancePropertiesVK.h"

namespace LeoVultana_VK
{
    void ExtVRSCheckExtensions(DeviceProperties* pDeviceProp, bool& Tier1Supported,  bool& Tier2Supported);
}