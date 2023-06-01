#pragma once

#include "RHI/Vulkan/VKCommon/DeviceVK.h"
#include "RHI/Vulkan/VKCommon/StaticBufferPoolVK.h"
#include "RHI/Vulkan/VKCommon/ResourceViewHeapsVK.h"
#include "RHI/Vulkan/VKCommon/DynamicBufferRingVK.h"
#include "GLTFTexturesAndBuffersVK.h"
#include "GLTF/GLTFPBRMaterial.h"
#include "RHI/Vulkan/VKCommon/GBufferVK.h"

namespace LeoVultana_VK
{

    struct BasePassMaterial
    {
        int mTextureCount = 0;
        VkDescriptorSet mTexturesDescSet{};
        VkDescriptorSetLayout mTextureDescSetLayout{};

        PBRMaterialParameters mBasePassMatParams;
    };

    struct BasePassPrimitives
    {
        Geometry mGeometry;
        BasePassMaterial* m_pMats{};

        VkPipeline mPipeline{};
        VkPipelineLayout mPipelineLayout{};

        VkDescriptorSet mUniformDescSet{};
        VkDescriptorSetLayout mUniformDescSetLayout{};

        void DrawPrimitive(
            VkCommandBuffer cmdBuffer,
            VkDescriptorBufferInfo perFrameDesc,
            VkDescriptorBufferInfo perObjectDesc
            );
    };
    struct BasePassMesh
    {
        std::vector<BasePassPrimitives> mPrimitives;
    };

    class GLTFBaseMeshPassVK
    {
    public:
        struct PerObjectData
        {
            math::Matrix4 mWorldViewProj;
            math::Vector4 mCenter;
            math::Vector4 mRadius;
            math::Vector4 mColor;
            PBRMaterialParametersConstantBuffer mPBRParams;
        };
        struct PerFrameData
        {
            math::Matrix4 mCurrentWorld;
            math::Matrix4 mPreviousWorld;
        };

        struct BatchList
        {
            float mDepth;
            BasePassPrimitives* mPrimitives;
            VkDescriptorBufferInfo  mPerFrameDesc;
            VkDescriptorBufferInfo  mPerObjectDesc;
            operator float() { return -mDepth; }
        };

    private:
        GLTFTexturesAndBuffers* m_pGLTFTexturesAndBuffers;

        ResourceViewHeaps*  m_pResourceViewHeaps;
        DynamicBufferRing*  m_pDynamicBufferRing;
        StaticBufferPool*   m_pStaticBufferPool;

        std::vector<BasePassMesh>     mMeshes;
        std::vector<BasePassMaterial> mMaterialDatas;

        BasePassMaterial mDefaultMaterial;

        Device*             m_pDevice;
        GBufferRenderPass*  m_pRenderPass;
        VkSampler           mSamplerPBR{};
        VkSampler           mSamplerShadow{};

        // PBR BRDF
        Texture        mBRDFLutTexture;
        VkImageView    mBRDFLutView{};
        VkSampler      mBRDFLutSampler{};
    };
}

