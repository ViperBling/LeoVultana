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

    class GLTFBaseMeshPass
    {
    public:
        struct PerObjectData
        {
            math::Matrix4 mCurrentWorld;
            math::Matrix4 mPreviousWorld;
            PBRMaterialParametersConstantBuffer mPBRParams;
        };

        struct BatchList
        {
            float mDepth;
            BasePassPrimitives* mPrimitives;
            VkDescriptorBufferInfo  mPerFrameDesc;
            VkDescriptorBufferInfo  mPerObjectDesc;
            operator float() { return -mDepth; }
        };

    public:
        void OnCreate(
            Device* pDevice,
            UploadHeap* pUploadHeap,
            ResourceViewHeaps* pResourceViewHeap,
            DynamicBufferRing* pDynamicBufferRing,
            StaticBufferPool* pStaticBufferPool,
            GLTFTexturesAndBuffers* pGLTFTexAndBuffers,
            std::vector<VkImageView>& ShadowMapViewPool,
            GBufferRenderPass* pRenderPass,
            AsyncPool* pAsyncPool = nullptr
            );
        void OnDestroy();
        void BuildBatchList(
            std::vector<BatchList>* pSolid,
            std::vector<BatchList>* pTransparent
            );
        void DrawBatchList(VkCommandBuffer cmdBuffer, std::vector<BatchList>* pBatchList);

    private:
        void CreateDescTableForMaterialTextures(
            BasePassMaterial* gltfMat,
            std::map<std::string, VkImageView>& textureBase,
            std::vector<VkImageView>& ShadowMapViewPool
            );
        void CreateDescriptors(
            int inverseMatrixBufferSize,
            DefineList* pAttributeDefines,
            BasePassPrimitives* pPrimitive
            );
        void CreatePipeline(
            std::vector<VkVertexInputAttributeDescription> layout,
            const DefineList& defines,
            BasePassPrimitives* pPrimitive
            );

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
        VkSampler           mSamplerMat{};
        VkSampler           mSamplerShadow{};

        // PBR BRDF
        Texture        mBRDFLutTexture;
        VkImageView    mBRDFLutView{};
        VkSampler      mBRDFLutSampler{};
    };
}

