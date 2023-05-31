#pragma once

#include <sdkddkver.h>

//#define NOMINMAX
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <Windows.h>
#include <windowsx.h>
#include <wrl.h>
#include <KnownFolders.h>
#include <ShlObj.h>

// C RunTime Header Files
#include <malloc.h>
#include <tchar.h>

// Gfx API
#include "Vulkan/vulkan.h"
#include "Vulkan/vulkan_win32.h"

// math API
#include <DirectXMath.h>
using namespace DirectX;

#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <mutex>
#include <limits>
#include <algorithm>
