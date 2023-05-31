#pragma once

#include "PostProcessPS.h"
#include "RHI/Vulkan/TextureVK.h"
#include "RHI/Vulkan/UploadHeapVK.h"
#include "vectormath/vectormath.hpp"

namespace LeoVultana_VK
{
    class SkyDome
    {
    public:
        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            UploadHeap* pUploadHeap,
            VkFormat outFormat,
            ResourceViewHeaps *pResourceViewHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool  *pStaticBufferPool,
            const char *pDiffuseCubeMap,
            const char *pSpecularCubeMap,
            VkSampleCountFlagBits sampleDescCount);

        void OnDestroy();
        void Draw(VkCommandBuffer cmdBuffer, const math::Matrix4& invViewProj);
        void GenerateDiffuseMapFromEnvironmentMap();

        void SetDescriptorDiffuse(uint32_t index, VkDescriptorSet descriptorSet);
        void SetDescriptorSpecular(uint32_t index, VkDescriptorSet descriptorSet);

        VkImageView GetCubeDiffuseTextureView() const { return mCubeDiffuseTextureView; }
        VkImageView GetCubeSpecularTextureView() const { return mCubeSpecularTextureView; }
        VkSampler GetCubeDiffuseTextureSampler() const { return mSamplerDiffuseCube; }
        VkSampler GetCubeSpecularTextureSampler() const { return mSamplerSpecularCube; }

    private:
        Device*             m_pDevice;
        ResourceViewHeaps*  m_pResourceViewHeaps;
        DynamicBufferRing*  m_pDynamicBufferRing;

        Texture     mCubeDiffuseTexture;
        Texture     mCubeSpecularTexture;
        VkImageView mCubeDiffuseTextureView;
        VkImageView mCubeSpecularTextureView;

        VkSampler mSamplerDiffuseCube;
        VkSampler mSamplerSpecularCube;

        VkDescriptorSet       mDescSet;
        VkDescriptorSetLayout mDescSetLayout;

        PostProcessPS  mSkyDome;
    };
}