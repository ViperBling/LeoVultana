#pragma once

#include "Function/Misc.h"
#include "FreeSyncHDRVK.h"
#include "DeviceVK.h"
#include "SwapChainVK.h"

namespace LeoVultana_VK
{

    class FrameworkWindows
    {
    public:
        FrameworkWindows(LPCSTR name);
        virtual ~FrameworkWindows() {}

        // Client application interface
        virtual void OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight) = 0;
        virtual void OnCreate() = 0;
        virtual void OnDestroy() = 0;
        virtual void OnRender() = 0;
        virtual bool OnEvent(MSG msg) = 0;
        virtual void OnResize(bool resizeRender) = 0;
        virtual void OnUpdateDisplay() = 0;

        // Device & swapchain management
        void DeviceInit(HWND hWnd);
        void DeviceShutdown();
        void BeginFrame();
        void EndFrame();

        // Fullscreen management & window events are handled by Libs instead of the client applications.
        void ToggleFullScreen();
        void HandleFullScreen();
        void OnActivate(bool windowActive);
        void OnWindowMove();
        void UpdateDisplay();
        void OnResize(uint32_t Width, uint32_t Height);
        void OnLocalDimmingChanged();

        // Getters
        inline LPCSTR             GetName() const { return mName; }
        inline uint32_t           GetWidth() const { return mWidth; }
        inline uint32_t           GetHeight() const { return mHeight; }
        inline DisplayMode        GetCurrentDisplayMode() const { return mCurrentDisplayModeNamesIndex; }
        inline size_t             GetNumDisplayModes() const { return mDisplayModesAvailable.size(); }
        inline const char* const* GetDisplayModeNames() const { return reinterpret_cast<const char *const *>(&mDisplayModesAvailable[0]); }
        inline bool               GetLocalDimmingEnableState() const { return mEnableLocalDimming; }

    protected:
        LPCSTR     mName;
        uint32_t   mWidth;
        uint32_t   mHeight;

        // Simulation Management
        double mLastFrameTime;
        double mDeltaTime;

        // Device Management
        HWND        mWindowHWND;
        Device      mDevice;
        bool        mStablePowerState;
        bool        mIsCPUValidationLayerEnabled;
        bool        mIsGPUValidationLayerEnabled;

        // SwapChain Management
        SwapChain           mSwapChain;
        bool                mVsyncEnabled;
        PresentationMode    mFullScreenMode;
        PresentationMode    mPreviousFullScreenMode;

        // Dispaly Management
        HMONITOR                    mMonitor;
        bool                        mFreeSyncHDROptionEnabled;

        DisplayMode                 mPreviousDisplayModeNamesIndex;
        DisplayMode                 mCurrentDisplayModeNamesIndex;

        std::vector<DisplayMode>    mDisplayModesAvailable;
        std::vector<const char*>    mDisplayModesNamesAvailable;

        bool                        mEnableLocalDimming;

        // System Info
        struct SystemInfo
        {
            std::string mCPUName = "UNAVAILABLE";
            std::string mGPUName = "UNAVAILABLE";
            std::string mGfxAPI = "UNAVAILABLE";
        };
        SystemInfo mSystemInfo;
    };
}

using namespace LeoVultana_VK;

int RunFramework(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow, FrameworkWindows* pFrameworks);
void SetFullScreen(HWND hWnd, bool fullScreen);