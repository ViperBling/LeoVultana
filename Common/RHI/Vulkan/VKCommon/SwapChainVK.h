#pragma once

#include "DeviceVK.h"
#include "FreeSyncHDRVK.h"

namespace LeoVultana_VK
{
    class SwapChain
    {
    public:
        void OnCreate(Device* pDevice, uint32_t numBackBuffers, HWND hWnd);
        void OnDestroy();

        void OnCreateWindowSizeDependentResources(
            uint32_t dwWidth, uint32_t dwHeight,
            bool bVSyncOn, DisplayMode = DISPLAYMODE_SDR,
            PresentationMode fullScreenMode = PRESENTATIONMODE_WINDOWED,
            bool enableLocalDimming = true);
        void OnDestroyWindowSizeDependentResources();

        void SetFullScreen(bool fullscreen);

        bool IsModeSupported(
            DisplayMode displayMode,
            PresentationMode fullScreenMode = PRESENTATIONMODE_WINDOWED,
            bool enableLocalDimming = true);
        void EnumrateDisplayMode(
            std::vector<DisplayMode>* pModes,
            std::vector<const char*>* pNames = nullptr,
            bool includeFreeSyncHDR = false,
            PresentationMode fullScreenMode = PRESENTATIONMODE_WINDOWED,
            bool enableLocalDimming = true);

        void GetSemaphores(
            VkSemaphore* pImageAvailableSemaphore,
            VkSemaphore* pRenderFinishedSemaphores,
            VkFence* pCmdBufferExecuteFences);
        VkResult Present();
        uint32_t WaitForSwapChain();

        // Getters
        VkImage GetCurrentBackBuffer();
        VkImageView GetCurrentBackBufferRTV();
        VkSwapchainKHR GetSwapChain() const { return mSwapChain; }
        VkFormat GetFormat() const { return mSwapChainFormat.format; }
        VkRenderPass GetRenderPass() { return mRenderPassSwapChain; }
        DisplayMode GetDisplayMode() { return mDisplayMode; }
        VkFramebuffer GetFrameBuffer(int i) const { return mFrameBuffers[i]; }
        VkFramebuffer GetCurrentFrameBuffer() const { return mFrameBuffers[mImageIndex]; }

    private:
        void CreateRTV();
        void DestroyRTV();
        void CreateRenderPass();
        void DestroyRenderPass();
        void CreateFrameBuffers(uint32_t dwWidth, uint32_t dwHeight);
        void DestroyFrameBuffers();

    private:
        HWND mHwnd;
        Device* m_pDevice;

        VkSwapchainKHR mSwapChain;
        VkSurfaceFormatKHR mSwapChainFormat;

        VkQueue mPresentQueue;

        DisplayMode mDisplayMode = DISPLAYMODE_SDR;
        VkRenderPass mRenderPassSwapChain{};

        std::vector<VkImage> mImages;
        std::vector<VkImageView> mImageViews;
        std::vector<VkFramebuffer> mFrameBuffers;

        std::vector<VkFence> mCmdBufferExecutedFences;
        std::vector<VkSemaphore> mImageAvailableSemaphores;
        std::vector<VkSemaphore> mRenderFinishedSemaphores;

        uint32_t mImageIndex = 0;
        uint32_t mBackBufferCount{};
        uint32_t mSemaphoreIndex{}, mPrevSemaphoreIndex{};

        bool m_bVSyncOn = false;
    };
}