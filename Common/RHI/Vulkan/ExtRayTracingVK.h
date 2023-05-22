#pragma once

#include "DevicePropertiesVK.h"
#include "InstancePropertiesVK.h"

namespace LeoVultana_VK
{
    bool ExtRTCheckExtensions(DeviceProperties* pDP, bool& RT10Supported, bool& RT11Supported);
}