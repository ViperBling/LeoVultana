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

// TODO: reference additional headers your program requires here
#include "RHI/Vulkan/ImGuiVK.h"
#include "Utilities/ImGuiHelper.h"
#include "RHI/Vulkan/DeviceVK.h"
#include "RHI/Vulkan/HelperVK.h"
#include "RHI/Vulkan/TextureVK.h"
#include "RHI/Vulkan/GBufferVK.h"
#include "RHI/Vulkan/FrameworkWindowsVK.h"
#include "RHI/Vulkan/FreeSyncHDRVK.h"
#include "RHI/Vulkan/SwapChainVK.h"
#include "RHI/Vulkan/UploadHeapVK.h"
#include "RHI/Vulkan/GPUTimeStampsVK.h"
#include "RHI/Vulkan/CommandListRingVK.h"
#include "RHI/Vulkan/StaticBufferPoolVK.h"
#include "RHI/Vulkan/DynamicBufferRingVK.h"
#include "RHI/Vulkan/ResourceViewHeapsVK.h"
#include "RHI/Vulkan/ShaderCompilerHelperVK.h"
#include "Utilities/ShaderCompilerCache.h"


#include "RHI/Vulkan/GLTF_VK/GLTFPBRPassVK.h"
#include "RHI/Vulkan/GLTF_VK/GLTFBBoxPassVK.h"
#include "RHI/Vulkan/GLTF_VK/GLTFDepthPassVK.h"

#include "Function/Misc.h"
#include "Function/Camera.h"

#include "RHI/Vulkan/PostProcess/SkyDome.h"

#include "RHI/Vulkan/Widgets/AxisVK.h"
#include "RHI/Vulkan/Widgets/CheckerBoardFloorVK.h"
#include "RHI/Vulkan/Widgets/WireframeBoxVK.h"
#include "RHI/Vulkan/Widgets/WireframeSphereVK.h"

using namespace LeoVultana_VK;
