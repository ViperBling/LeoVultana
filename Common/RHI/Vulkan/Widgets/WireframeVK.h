#pragma once

#include "ResourceViewHeapsVK.h"
#include "DynamicBufferRingVK.h"
#include "StaticBufferPoolVK.h"
#include "vectormath/vectormath.hpp"

namespace LeoVultana_VK
{
    class Wireframe
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
        void Draw(
            VkCommandBuffer cmdBuffer,
            uint32_t numIndices,
            VkDescriptorBufferInfo IBV,
            VkDescriptorBufferInfo VBV,
            const math::Matrix4& worldMatrix,
            const math::Vector4& vCenter,
            const math::Vector4& vRadius,
            const math::Vector4& vColor);

    private:
        Device*                 m_pDevice;
        DynamicBufferRing*      m_pDynamicBufferRing;
        ResourceViewHeaps*      m_pResourceViewHeaps;

        VkPipeline              mPipeline;
        VkPipelineLayout        mPipelineLayout;

        VkDescriptorSet         mDescriptorSet;
        VkDescriptorSetLayout   mDescriptorSetLayout;

        struct perObject
        {
            math::Matrix4 mWorldViewProj;
            math::Vector4 mCenter;
            math::Vector4 mRadius;
            math::Vector4 mColor;
        };
    };
}