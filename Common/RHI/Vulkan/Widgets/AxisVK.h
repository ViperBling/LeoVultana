#pragma once

#include "ResourceViewHeapsVK.h"
#include "DynamicBufferRingVK.h"
#include "StaticBufferPoolVK.h"
#include "vectormath/vectormath.hpp"

namespace LeoVultana_VK
{
    // This class takes a GltfCommon class (that holds all the non-GPU specific data) as an input and loads all the GPU specific data
    class Axis
    {
    public:
        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            ResourceViewHeaps* pHeaps,
            DynamicBufferRing* pDynamicBufferRing,
            StaticBufferPool* pStaticBufferPool,
            VkSampleCountFlagBits sampleDescCount);
        void OnDestroy();
        void Draw(VkCommandBuffer cmdBuffer, const math::Matrix4& worldMatrix, const math::Matrix4& axisMatrix);

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
        };
    };
}