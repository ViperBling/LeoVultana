#pragma once

#include "DeviceVK.h"
#include "SwapChainVK.h"
#include "TextureVK.h"
#include "ExtDebugUtilsVK.h"
#include "ShaderCompiler.h"
#include "ResourceViewHeapsVK.h"

namespace LeoVultana_VK
{
    typedef enum GBufferFlagBits
    {
        GBUFFER_NONE = 0,
        GBUFFER_DEPTH = 1,
        GBUFFER_FORWARD = 2,
        GBUFFER_MOTION_VECTORS = 4,
        GBUFFER_NORMAL_BUFFER = 8,
        GBUFFER_DIFFUSE = 16,
        GBUFFER_SPECULAR_ROUGHNESS = 32
    } GBufferFlagBits;

    typedef uint32_t GBufferFlags;

    class GBuffer;

    class GBufferRenderPass
    {
    public:
        void OnCreate(GBuffer *pGBuffer, GBufferFlags flags, bool bClear, const std::string &name);
        void OnDestroy();

        void OnCreateWindowSizeDependentResources(uint32_t width, uint32_t height);
        void OnDestroyWindowSizeDependentResources();

        void BeginPass(VkCommandBuffer commandList, VkRect2D renderArea);
        void EndPass(VkCommandBuffer commandList);
        void GetCompilerDefines(DefineList &defines);
        VkRenderPass GetRenderPass() { return mRenderPass; }
        VkFramebuffer GetFramebuffer() { return mFrameBuffer; }
        VkSampleCountFlagBits  GetSampleCount();

    private:
        Device*                     m_pDevice;
        GBufferFlags                mFlags;
        GBuffer*                    m_pGBuffer;
        VkRenderPass                mRenderPass;
        VkFramebuffer               mFrameBuffer;
        std::vector<VkClearValue>   mClearValues;
    };

    class GBuffer
    {
    public:
        void OnCreate(Device* pDevice, ResourceViewHeaps *pHeaps, const std::map<GBufferFlags, VkFormat> &formats, int sampleCount);
        void OnDestroy();

        void OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t width, uint32_t height);
        void OnDestroyWindowSizeDependentResources();

        void GetAttachmentList(GBufferFlags flags, std::vector<VkImageView> *pAttachments, std::vector<VkClearValue> *pClearValues);
        VkRenderPass CreateRenderPass(GBufferFlags flags, bool bClear);

        VkSampleCountFlagBits  GetSampleCount() { return mSampleCount; }
        Device* GetDevice() { return m_pDevice; }

    public:
        // Depth Buffer
        Texture mDepthBuffer;
        VkImageView mDepthBufferDSV;
        VkImageView mDepthBufferSRV;

        // Diffuse
        Texture mDiffuse;
        VkImageView mDiffuseSRV;

        // Specular
        Texture mSpecularRoughness;
        VkImageView mSpecularRoughnessSRV;

        // Motion Vectors
        Texture mMotionVectors;
        VkImageView mMotionVectorsSRV;

        // Normal Buffer
        Texture mNormalBuffer;
        VkImageView mNormalBufferSRV;

        // HDR
        Texture mHDR;
        VkImageView mHDRSRV;

    private:
        Device*                             m_pDevice;
        VkSampleCountFlagBits               mSampleCount;
        GBufferFlags                        mGBufferFlags;
        std::vector<VkClearValue>           mClearValues;
        std::map<GBufferFlags, VkFormat>    mFormats;
    };
}