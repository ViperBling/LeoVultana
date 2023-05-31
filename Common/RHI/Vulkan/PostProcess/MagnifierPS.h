#pragma once

#include "PostProcessPS.h"
#include "RHI/Vulkan/VKCommon/TextureVK.h"

namespace LeoVultana_VK
{
    class DynamicBufferRing;
    class SwapChain;

    class MagnifierPS
    {
    public:
        struct PassParameters;
        void OnCreate(
            Device* pDevice
            , ResourceViewHeaps* pResourceViewHeaps
            , DynamicBufferRing* pDynamicBufferRing
            , StaticBufferPool* pStaticBufferPool
            , VkFormat outFormat
            , bool bOutputsToSwapChain = false
        );

        void OnDestroy();
        void OnCreateWindowSizeDependentResources(Texture* pTexture);
        void OnDestroyWindowSizeDependentResources();
        void BeginPass(VkCommandBuffer cmdBuffer, VkRect2D renderArea, SwapChain* pSwapChain = nullptr);
        void EndPass(VkCommandBuffer cmdBuffer);

        // Draws a magnified region on top of the given image to OnCreateWindowSizeDependentResources()
        // if @pSwapChain is provided, expects swapchain to be sycned before this call.
        // Barriers are to be managed by the caller.
        void Draw(VkCommandBuffer cmdBuffer, const PassParameters& params, SwapChain* pSwapChain = nullptr);

        inline Texture& GetPassOutput() { assert(!m_bOutToSwapChain); return mTexturePassOut; }
        inline VkImage GetPassOutputResource() const { assert(!m_bOutToSwapChain); return mTexturePassOut.Resource(); }
        inline VkImageView GetPassOutputSRV() const  { assert(!m_bOutToSwapChain); return mSRVOut; }
        inline VkRenderPass GetPassRenderPass() const { assert(!m_bOutToSwapChain); return mRenderPass; }
        void UpdatePipelines(VkRenderPass renderPass);

    public:
        struct PassParameters
        {
            PassParameters() :
                uImageWidth(1),
                uImageHeight(1),
                iMousePos{0, 0},
                fBorderColorRGB{1, 1, 1, 1},
                fMagnificationAmount(6.0f),
                fMagnifierScreenRadius(0.35f),
                iMagnifierOffset{500, -500}
            {}

            uint32_t    uImageWidth;
            uint32_t    uImageHeight;
            int         iMousePos[2];            // in pixels, driven by ImGuiIO.MousePos.xy
            float       fBorderColorRGB[4];      // Linear RGBA
            float       fMagnificationAmount;    // [1-...]
            float       fMagnifierScreenRadius;  // [0-1]
            mutable int iMagnifierOffset[2];     // in pixels
        };

    private:
        void CompileShaders(StaticBufferPool* pStaticBufferPool, VkFormat outFormat);
        void DestroyShaders();

        void InitializeDescriptorSets();
        void UpdateDescriptorSets(VkImageView ImageViewSrc);
        void DestroyDescriptorSets();
        VkDescriptorBufferInfo SetConstantBufferData(const PassParameters& params);
        static void KeepMagnifierOnScreen(const PassParameters& params);

    private:
        Device* m_pDevice;
        ResourceViewHeaps* m_pResourceViewHeaps;
        DynamicBufferRing* m_pDynamicBufferRing;

        VkDescriptorSet mDescSet;
        VkDescriptorSetLayout mDescSetLayout;

        VkSampler mSampler;
        VkImageView mImageView;

        VkRenderPass mRenderPass;
        VkFramebuffer mFrameBuffer;
        VkImageView mRTVOut;
        VkImageView mSRVOut;

        Texture mTexturePassOut;

        PostProcessPS mShaderMagnify;

        bool m_bOutToSwapChain = false;
    };
}