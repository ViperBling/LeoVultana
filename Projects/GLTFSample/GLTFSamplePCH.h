// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <windowsx.h>

// C RunTime Header Files
#include <malloc.h>
#include <map>
#include <vector>
#include <mutex>
#include <fstream>
#include <minwindef.h>
#include <algorithm>
#include <limits>

#include "vulkan/vulkan.h"

// Pull in math library
#include "vectormath/vectormath.hpp"

#include "RHI/Vulkan/VKCommon/ImGuiVK.h"
#include "RHI/Vulkan/VKCommon/DeviceVK.h"
#include "RHI/Vulkan/VKCommon/InstanceVK.h"
#include "RHI/Vulkan/VKCommon/HelperVK.h"
#include "RHI/Vulkan/VKCommon/TextureVK.h"
#include "RHI/Vulkan/VKCommon/GBufferVK.h"
#include "RHI/Vulkan/VKCommon/FrameworkWindowsVK.h"
#include "RHI/Vulkan/VKCommon/FreeSyncHDRVK.h"
#include "RHI/Vulkan/VKCommon/SwapChainVK.h"
#include "RHI/Vulkan/VKCommon/UploadHeapVK.h"
#include "RHI/Vulkan/VKCommon/GPUTimeStampsVK.h"
#include "RHI/Vulkan/VKCommon/CommandListRingVK.h"
#include "RHI/Vulkan/VKCommon/StaticBufferPoolVK.h"
#include "RHI/Vulkan/VKCommon/DynamicBufferRingVK.h"
#include "RHI/Vulkan/VKCommon/ResourceViewHeapsVK.h"
#include "RHI/Vulkan/VKCommon/ShaderCompilerHelperVK.h"
#include "Utilities/ShaderCompilerCache.h"


#include "RHI/Vulkan/GLTF_VK/GLTFPBRPassVK.h"
#include "RHI/Vulkan/GLTF_VK/GLTFBBoxPassVK.h"
#include "RHI/Vulkan/GLTF_VK/GLTFBaseMeshPassVK.h"
#include "RHI/Vulkan/GLTF_VK/GLTFDepthPassVK.h"

#include "Utilities/Misc.h"
#include "Utilities/Camera.h"

#include "RHI/Vulkan/PostProcess/SkyDome.h"

#include "RHI/Vulkan/Widgets/AxisVK.h"
#include "RHI/Vulkan/Widgets/CheckerBoardFloorVK.h"
#include "RHI/Vulkan/Widgets/WireframeBoxVK.h"
#include "RHI/Vulkan/Widgets/WireframeSphereVK.h"

#include "Utilities/ImGuiHelper.h"

using namespace LeoVultana_VK;
