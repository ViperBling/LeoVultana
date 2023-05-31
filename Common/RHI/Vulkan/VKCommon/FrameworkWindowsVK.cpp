#include "PCHVK.h"
#include "FrameworkWindowsVK.h"
#include "Misc.h"
#include "HelperVK.h"

#include <array>

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static const char* const WINDOW_CLASS_NAME = "LeoVultana";
static FrameworkWindows* pFrameworkInstance = nullptr;
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

int RunFramework(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow, FrameworkWindows* pFramework)
{
    // Init Logging
    assert(!Log::InitLogSystem());

    HWND hWnd;
    WNDCLASSEX windowClass;

    ZeroMemory(&windowClass, sizeof(WNDCLASSEX));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    windowClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101)); // '#define ICON_IDI 101' is assumed to be included with resource.h
    if (windowClass.hIcon == nullptr)
    {
        DWORD dw = GetLastError();
        if (dw == ERROR_RESOURCE_TYPE_NOT_FOUND)
            Trace("Warning: Icon file or .rc file not found, using default Windows app icon.");
        else
            Trace("Warning: error loading icon, using default Windows app icon.");
    }
    RegisterClassEx(&windowClass);
    // If this is null, nothing to do, bail
    assert(pFramework);
    if (!pFramework) return -1;
    pFrameworkInstance = pFramework;

    // Get command line and config file parameters for app run
    uint32_t Width = 1280;
    uint32_t Height = 720;
    pFramework->OnParseCommandLine(lpCmdLine, &Width, &Height);

    // Window setup based on config params
    lWindowStyle = WS_OVERLAPPEDWINDOW;
    RECT windowRect = { 0, 0, (LONG)Width, (LONG)Height };
    AdjustWindowRect(&windowRect, lWindowStyle, FALSE);    // adjust the size

    // This makes sure that in a multi-monitor setup with different resolutions, get monitor info returns correct dimensions
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Create the window
    hWnd = CreateWindowEx(
        NULL,
        WINDOW_CLASS_NAME,    // name of the window class
        pFramework->GetName(),
        lWindowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,    // we have no parent window, NULL
        nullptr,    // we aren't using menus, NULL
        hInstance,    // application handle
        nullptr);    // used with multiple windows, NULL

    // Framework owns device and swapchain, so initialize them
    pFramework->DeviceInit(hWnd);

    // Sample create callback
    pFramework->OnCreate();

    // Show window
    ShowWindow(hWnd, nCmdShow);
    lBorderedStyle = GetWindowLong(hWnd, GWL_STYLE);
    lBorderlessStyle = lBorderedStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

    // main loop
    MSG msg = { nullptr };
    while (msg.message != WM_QUIT)
    {
        // check to see if any messages are waiting in the queue
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); // translate keystroke messages into the right format
            DispatchMessage(&msg); // send the message to the WindowProc function
        }
        else if (!bIsMinimized)
            pFramework->OnRender();
    }

    // Destroy app-side framework
    pFramework->OnDestroy();

    // Shutdown all the device stuff before quitting
    pFramework->DeviceShutdown();

    // Delete the framework created by the sample
    pFrameworkInstance = nullptr;
    delete pFramework;

    // Shutdown logging before quitting the application
    Log::TerminateLogSystem();

    // return this part of the WM_QUIT message to Sample
    return static_cast<char>(msg.wParam);
}

void SetFullScreen(HWND hWnd, bool fullScreen)
{
    if (fullScreen)
    {
        // Save the old window rect, so we can restore it when exiting fullscreen mode.
        GetWindowRect(hWnd, &mWindowRect);

        // Make the window borderless so that the client area can fill the screen.
        SetWindowLong(hWnd, GWL_STYLE, (LONG)lWindowStyle & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME));

        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(monitorInfo);
        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &monitorInfo);

        SetWindowPos(
            hWnd,
            HWND_NOTOPMOST,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(hWnd, SW_MAXIMIZE);
    }
    else
    {
        // Restore the window's attributes and size.
        SetWindowLong(hWnd, GWL_STYLE, (LONG)lWindowStyle);

        SetWindowPos(
            hWnd,
            HWND_NOTOPMOST,
            mWindowRect.left,
            mWindowRect.top,
            mWindowRect.right - mWindowRect.left,
            mWindowRect.bottom - mWindowRect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(hWnd, SW_NORMAL);
    }
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // sort through and find what code to run for the message given
    switch (message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        // When close button is clicked on window
        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }
            break;
        }
        case WM_SYSKEYDOWN:
        {
            const bool bAltKeyDown = (lParam & (1 << 29));
            if ((wParam == VK_RETURN) && bAltKeyDown) // For simplicity, alt+enter only toggles in/out windowed and borderless fullscreen
                pFrameworkInstance->ToggleFullScreen();
            break;
        }
        case WM_SIZE:
        {
            if (pFrameworkInstance)
            {
                RECT clientRect = {};
                GetClientRect(hWnd, &clientRect);
                pFrameworkInstance->OnResize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
                bIsMinimized = (IsIconic(hWnd) == TRUE);
                return 0;
            }
            break;
        }
        // When window goes outof focus, use this event to fall back on SDR.
        // If we don't gracefully fall back to SDR, the renderer will output HDR colours which will look extremely bright and washed out.
        // However, if you want to use breakpoints in HDR mode to inspect/debug values, you will have to comment this function call.
        case WM_ACTIVATE:
        {
            if (pFrameworkInstance)
            {
                pFrameworkInstance->OnActivate(wParam != WA_INACTIVE);
            }
            break;
        }
        case WM_MOVE:
        {
            if (pFrameworkInstance)
            {
                pFrameworkInstance->OnWindowMove();

                return 0;
            }
            break;
        }
        // Turn off MessageBeep sound on Alt+Enter
        case WM_MENUCHAR: return MNC_CLOSE << 16;
        default:
            break;
    }

    if (pFrameworkInstance)
    {
        MSG msg;
        msg.hwnd = hWnd;
        msg.message = message;
        msg.wParam = wParam;
        msg.lParam = lParam;
        pFrameworkInstance->OnEvent(msg);
    }
    // Handle any messages the switch statement didn't
    return DefWindowProc(hWnd, message, wParam, lParam);
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

FrameworkWindows::FrameworkWindows(LPCSTR name) :
    mName(name), mWidth(0), mHeight(0),
    // Simulation Manage
    mLastFrameTime(MillisecondsNow()),
    mDeltaTime(0.0),
    // Device Manage
    mWindowHWND(nullptr), mDevice(), mStablePowerState(false),
    mIsCPUValidationLayerEnabled(ENABLE_CPU_VALIDATION_DEFAULT),
    mIsGPUValidationLayerEnabled(ENABLE_GPU_VALIDATION_DEFAULT),
    // SwapChain Manage
    mSwapChain(), mVsyncEnabled(false),
    mFullScreenMode(PRESENTATIONMODE_WINDOWED),
    mPreviousFullScreenMode(PRESENTATIONMODE_WINDOWED),
    // Display Manage
    mMonitor(), mFreeSyncHDROptionEnabled(false),
    mPreviousDisplayModeNamesIndex(DISPLAYMODE_SDR),
    mCurrentDisplayModeNamesIndex(DISPLAYMODE_SDR),
    mDisplayModesAvailable(), mDisplayModesNamesAvailable(),
    mEnableLocalDimming(true),
    // SystemInfo
    mSystemInfo()
{
}

void FrameworkWindows::DeviceInit(HWND hWnd)
{
    // 存储窗口句柄
    mWindowHWND = hWnd;

    // 创建设备
    mDevice.OnCreate(mName, "LeoVultana", mIsCPUValidationLayerEnabled, mIsGPUValidationLayerEnabled, mWindowHWND);
    mDevice.CreatePipelineCache();

    // 获取Monitor
    mMonitor = MonitorFromWindow(mWindowHWND, MONITOR_DEFAULTTONEAREST);

    // SwapChain
    uint32_t dwNumberOfBackBuffers = 2;
    mSwapChain.OnCreate(&mDevice, dwNumberOfBackBuffers, mWindowHWND);
    mSwapChain.EnumrateDisplayMode(&mDisplayModesAvailable, &mDisplayModesNamesAvailable);
    if (mPreviousFullScreenMode != mFullScreenMode)
    {
        HandleFullScreen();
        mPreviousFullScreenMode = mFullScreenMode;
    }

    // System Info
    std::string dummyStr;
    mDevice.GetDeviceInfo(&mSystemInfo.mGPUName, &dummyStr);
    mSystemInfo.mCPUName = GetCPUNameString();
    mSystemInfo.mGfxAPI = "Vulkan";
}

void FrameworkWindows::DeviceShutdown()
{
    // 在退出之前不能是FullScreen
    if (mFullScreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN) mSwapChain.SetFullScreen(false);
    mSwapChain.OnDestroyWindowSizeDependentResources();
    mSwapChain.OnDestroy();

    mDevice.DestroyPipelineCache();
    mDevice.OnDestroy();
}

void FrameworkWindows::BeginFrame()
{
    double timeNow = MillisecondsNow();
    mDeltaTime = (timeNow - mLastFrameTime);
    mLastFrameTime = timeNow;
}

void FrameworkWindows::EndFrame()
{
    VkResult res = mSwapChain.Present();
    // *********************************************************************************
    // Edge case for handling Fullscreen Exclusive (FSE) mode
    // Usually OnActivate() detects application changing focus and that's where this transition is handled.
    // However, SwapChain::GetFullScreen() returns true when we handle the event in the OnActivate() block,
    // which is not expected. Hence, we handle FSE -> FSB transition here at the end of the frame.
    if (res == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
    {
        assert(mFullScreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN);
        mFullScreenMode = PRESENTATIONMODE_BORDERLESS_FULLSCREEN;
        HandleFullScreen();
    }
    // *********************************************************************************
}

void FrameworkWindows::ToggleFullScreen()
{
    if (mFullScreenMode == PRESENTATIONMODE_WINDOWED) mFullScreenMode = PRESENTATIONMODE_BORDERLESS_FULLSCREEN;
    else mFullScreenMode = PRESENTATIONMODE_WINDOWED;

    HandleFullScreen();
    mPreviousFullScreenMode = mFullScreenMode;
}

void FrameworkWindows::HandleFullScreen()
{
    // 刷新GPU状态
    mDevice.GPUFlush();

    // ------------ For HDR only ---------------------- //
    // If FS2 modes, always fallback to SDR
    if (mFullScreenMode == PRESENTATIONMODE_WINDOWED &&
        (mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] == DISPLAYMODE_FSHDR_Gamma22 ||
         mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] == DISPLAYMODE_FSHDR_SCRGB))
    {
        mCurrentDisplayModeNamesIndex = DISPLAYMODE_SDR;
    }
        // when hdr10 modes, fall back to SDR unless windowMode hdr is enabled
    else if (mFullScreenMode == PRESENTATIONMODE_WINDOWED && !CheckIfWindowModeHDROn() &&
             mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] != DISPLAYMODE_SDR)
    {
        mCurrentDisplayModeNamesIndex = DISPLAYMODE_SDR;
    }
    // For every other case go back to previous state
    else
    {
        mCurrentDisplayModeNamesIndex = mPreviousDisplayModeNamesIndex;
    }
    // ------------ For HDR only ---------------------- //

    bool resizeResources = false;
    switch (mFullScreenMode)
    {
        case PRESENTATIONMODE_WINDOWED:
        {
            if (mPreviousFullScreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN) mSwapChain.SetFullScreen(false);
            SetFullScreen(mWindowHWND, false);
            break;
        }
        case PRESENTATIONMODE_BORDERLESS_FULLSCREEN:
        {
            if (mPreviousFullScreenMode == PRESENTATIONMODE_WINDOWED)
            {
                SetFullScreen(mWindowHWND, true);
            }
            else if (mPreviousFullScreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN)
            {
                mSwapChain.SetFullScreen(false);
                resizeResources = true;
            }
            break;
        }
        case PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN:
        {
            if (mPreviousFullScreenMode == PRESENTATIONMODE_WINDOWED) SetFullScreen(mWindowHWND, true);
            else if (mPreviousFullScreenMode == PRESENTATIONMODE_BORDERLESS_FULLSCREEN) resizeResources = true;
            mSwapChain.SetFullScreen(true);
            break;
        }
        default:
            break;
    }
    RECT clientRect{};
    GetClientRect(mWindowHWND, &clientRect);
    uint32_t nW = clientRect.right - clientRect.left;
    uint32_t nH = clientRect.bottom - clientRect.top;
    resizeResources = (resizeResources && nW == mWidth && nH == mHeight);
    OnResize(nW, nH);
    if (resizeResources)
    {
        UpdateDisplay();
        OnResize(true);
    }
}

void FrameworkWindows::OnActivate(bool windowActive)
{
    // *********************************************************************************
    // Edge case for handling Fullscreen Exclusive (FSE) mode
    // FSE<->FSB transitions are handled here and at the end of EndFrame().
    if (windowActive &&
        mWindowHWND == GetForegroundWindow() &&
        mFullScreenMode == PRESENTATIONMODE_BORDERLESS_FULLSCREEN &&
        mPreviousFullScreenMode == PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN)
    {
        mFullScreenMode = PRESENTATIONMODE_EXCLUSIVE_FULLSCREEN;
        mPreviousFullScreenMode = PRESENTATIONMODE_BORDERLESS_FULLSCREEN;
        HandleFullScreen();
        mPreviousFullScreenMode = mFullScreenMode;
    }
    // *********************************************************************************

    if (mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] == DisplayMode::DISPLAYMODE_SDR &&
        mDisplayModesAvailable[mPreviousDisplayModeNamesIndex] == DisplayMode::DISPLAYMODE_SDR)
        return;

    if (CheckIfWindowModeHDROn() &&
        (mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] == DISPLAYMODE_HDR10_2084 ||
         mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] == DISPLAYMODE_HDR10_SCRGB))
        return;

    // Fall back HDR to SDR when window is fullscreen but not the active window or foreground window
    DisplayMode old = mCurrentDisplayModeNamesIndex;
    mCurrentDisplayModeNamesIndex = windowActive && (mFullScreenMode != PRESENTATIONMODE_WINDOWED) ? mPreviousDisplayModeNamesIndex : DisplayMode::DISPLAYMODE_SDR;
    if (old != mCurrentDisplayModeNamesIndex)
    {
        UpdateDisplay();
        OnResize(true);
    }
}

void FrameworkWindows::OnWindowMove()
{
    HMONITOR currentMonitor = MonitorFromWindow(mWindowHWND, MONITOR_DEFAULTTONEAREST);
    if (mMonitor != currentMonitor)
    {
        mSwapChain.EnumrateDisplayMode(&mDisplayModesAvailable, &mDisplayModesNamesAvailable);
        mMonitor = currentMonitor;
        mPreviousDisplayModeNamesIndex = mCurrentDisplayModeNamesIndex = DISPLAYMODE_SDR;
        OnResize(mWidth, mHeight);
        UpdateDisplay();
    }
}

void FrameworkWindows::UpdateDisplay()
{
    // Nothing was changed in UI
    if (mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] < 0)
    {
        mCurrentDisplayModeNamesIndex = mPreviousDisplayModeNamesIndex;
        return;
    }

    // Flush GPU
    mDevice.GPUFlush();

    mSwapChain.OnDestroyWindowSizeDependentResources();

    mSwapChain.OnCreateWindowSizeDependentResources(
        mWidth, mHeight, mVsyncEnabled,
        mDisplayModesAvailable[mCurrentDisplayModeNamesIndex],
        mFullScreenMode, mEnableLocalDimming);

    // Call sample defined UpdateDisplay()
    OnUpdateDisplay();
}

void FrameworkWindows::OnResize(uint32_t Width, uint32_t Height)
{
    bool fr = (mWidth != Width || mHeight != Height);
    mWidth = Width;
    mHeight = Height;
    if (fr)
    {
        UpdateDisplay();
        OnResize(true);
    }
}

void FrameworkWindows::OnLocalDimmingChanged()
{
    // Flush GPU
    mDevice.GPUFlush();
    FSHDRSetLocalDimmingMode(mSwapChain.GetSwapChain(), mEnableLocalDimming);
}
