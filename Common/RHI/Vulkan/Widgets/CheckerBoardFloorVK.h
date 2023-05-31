#pragma once

#include "RHI/Vulkan/VKCommon/ResourceViewHeapsVK.h"
#include "RHI/Vulkan/VKCommon/DynamicBufferRingVK.h"
#include "RHI/Vulkan/VKCommon/StaticBufferPoolVK.h"
#include "vectormath/vectormath.hpp"

namespace LeoVultana_VK
{
    // 棋盘地板
    class CheckerBoardFloor
    {
    public:
        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            VkSampleCountFlagBits sampleDescCount);

        void OnDestroy();
        void Draw(VkCommandBuffer cmdBuffer, const math::Matrix4& worldMatrix, const math::Vector4& color);

    private:
        Device*                 m_pDevice;
        DynamicBufferRing*      m_pDynamicBufferRing;
        StaticBufferPool*       m_pStaticBufferPool;
        ResourceViewHeaps*      m_pResourceViewHeaps;

        // all bounding boxes of all the meshes use the same geometry, shaders and pipelines.
        uint32_t                mNumIndices;
        VkIndexType             mIndexType;
        VkDescriptorBufferInfo  mIBV;        // Index Buffer
        VkDescriptorBufferInfo  mVBV;        // Vertex Buffer

        VkPipeline              mPipeline;
        VkPipelineLayout        mPipelineLayout;

        VkDescriptorSet         mDescriptorSet;
        VkDescriptorSetLayout   mDescriptorSetLayout;

        struct perObject
        {
            math::Matrix4 mWorldViewProj;
            math::Vector4 mColor;
        };
    };
}