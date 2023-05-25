#include "PCHVK.h"
#include "FrameworkWindowsVK.h"
#include "Misc.h"
#include "HelperVK.h"

#include <array>

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static const char* const WINDOW_CLASS_NAME = "LeoVultana";
static FrameworkWindows* pFrameworkInterface = nullptr;
static bool bIsMinimized = false;
static RECT mWindowRect{};
static LONG lBorderedStyle{};
static LONG lBorderlessStyle{};
static UINT lWindowStyle{};

// Default values for validation layers - applications can override these values in their constructors
#if _DEBUG
static constexpr bool ENABLE_CPU_VALIDATION_DEFAULT = true;
static constexpr bool ENABLE_GPU_VALIDATION_DEFAULT = true;
#else // RELEASE
static constexpr bool ENABLE_CPU_VALIDATION_DEFAULT = false;
static constexpr bool ENABLE_GPU_VALIDATION_DEFAULT = false;
#endif

int RunFramework(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow, FrameworkWindows* pFrameworks)
{

}

void SetFullScreen(HWND hWnd, bool fullScreen)
{

}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

}

static std::string GetCPUNameString()
{
    int nIDs = 0;
    int nExIDs = 0;

    char strCPUName[0x40]{};

    std::array<int , 4> cpuInfo{};
    std::vector<std::array<int, 4>> extData;

    __cpuid(cpuInfo.data(), 0);

    // Calling __cpuid with 0x80000000 as the function_id argument
    // gets the number of the highest valid extended ID.
    __cpuid(cpuInfo.data(), 0x80000000);

    nExIDs = cpuInfo[0];
    for (int i = 0x80000000; i <= nExIDs; ++i)
    {
        __cpuidex(cpuInfo.data(), i, 0);
        extData.push_back(cpuInfo);
    }

    // Interpret CPU strCPUName string if reported
    if (nExIDs >= 0x80000004)
    {
        memcpy(strCPUName, extData[2].data(), sizeof(cpuInfo));
        memcpy(strCPUName + 16, extData[3].data(), sizeof(cpuInfo));
        memcpy(strCPUName + 32, extData[4].data(), sizeof(cpuInfo));
    }

    return strlen(strCPUName) != 0 ? strCPUName : "UNAVAILABLE";

}

FrameworkWindows::FrameworkWindows(LPCSTR name)
{

}

void FrameworkWindows::DeviceInit(HWND hWnd)
{

}

void FrameworkWindows::DeviceShutdown()
{

}

void FrameworkWindows::BeginFrame()
{

}

void FrameworkWindows::EndFrame()
{

}

void FrameworkWindows::ToggleFullScreen()
{

}

void FrameworkWindows::HandleFullScreen()
{

}

void FrameworkWindows::OnActivate(bool windowActive)
{

}

void FrameworkWindows::OnWindowMove()
{

}

void FrameworkWindows::UpdateDisplay()
{

}

void FrameworkWindows::OnResize(uint32_t Width, uint32_t Height)
{

}

void FrameworkWindows::OnLocalDimmingChanged()
{

}
