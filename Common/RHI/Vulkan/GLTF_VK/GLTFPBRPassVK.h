#pragma once

#include "GLTFTexturesAndBuffersVK.h"
#include "RHI/Vulkan/GBufferVK.h"
#include "GLTF/GLTFPBRMaterial.h"
#include "RHI/Vulkan/PostProcess/SkyDome.h"

namespace LeoVultana_VK
{
    struct PBRMaterial
    {
        int mTextureCount = 0;
        VkDescriptorSet mTextureDescSet{};
        VkDescriptorSetLayout mTextureDescSetLayout{};

        PBRMaterialParameters mPBRMaterialParameters;
    };

    struct PBRPrimitives
    {
        Geometry mGeometry;
        PBRMaterial* m_pMaterial{};

        VkPipeline mPipeline{};
        VkPipeline mPipelineWireframe{};
        VkPipelineLayout mPipelineLayout{};

        VkDescriptorSet mUniformDescSet{};
        VkDescriptorSetLayout mUniformDescSetLayout{};

        void DrawPrimitive(
            VkCommandBuffer cmdBuffer,
            VkDescriptorBufferInfo perFrameDesc,
            VkDescriptorBufferInfo perObjectDesc,
            VkDescriptorBufferInfo *pPerSkeleton,
            bool bWireframe);
    };

    struct PBRMesh
    {
        std::vector<PBRPrimitives> mPrimitives;
    };

    class GLTFPBRPass
    {
    public:
        struct PerObject
        {
            math::Matrix4 mCurrentWorld;
            math::Matrix4 mPreviousWorld;
            PBRMaterialParametersConstantBuffer mPBRParams;
        } mPerObject;

        struct PerFrame
        {
            math::Matrix4 mCurrentWorld;
            math::Matrix4 mPreviousWorld;
        } mPerFrame;

        struct BatchList
        {
            float mDepth;
            PBRPrimitives* m_pPrimitive;
            VkDescriptorBufferInfo mPerFrameDesc;
            VkDescriptorBufferInfo mPerObjectDesc;
            VkDescriptorBufferInfo* m_pPerSkeleton;
            operator float() { return -mDepth; }
        } mBatchList;

    public:
        void OnCreate(
            Device* pDevice,
            UploadHeap* pUploadHeap,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
            SkyDome *pSkyDome,
            bool bUseSSAOMask,
            std::vector<VkImageView>& ShadowMapViewPool,
            GBufferRenderPass *pRenderPass,
            AsyncPool *pAsyncPool = nullptr
        );

        void OnDestroy();
        void BuildBatchLists(std::vector<BatchList> *pSolid, std::vector<BatchList> *pTransparent, bool bWireframe = false);
        void DrawBatchList(VkCommandBuffer commandBuffer, std::vector<BatchList> *pBatchList, bool bWireframe = false);
        void OnUpdateWindowSizeDependentResources(VkImageView SSAO);

    private:
        void CreateDescriptorTableForMaterialTextures(
            PBRMaterial *tfMat,
            std::map<std::string, VkImageView> &texturesBase,
            SkyDome *pSkyDome,
            std::vector<VkImageView>& ShadowMapViewPool,
            bool bUseSSAOMask
            );
        void CreateDescriptors(
            int inverseMatrixBufferSize,
            DefineList *pAttributeDefines,
            PBRPrimitives *pPrimitive,
            bool bUseSSAOMask
            );
        void CreatePipeline(
            std::vector<VkVertexInputAttributeDescription> layout,
            const DefineList &defines,
            PBRPrimitives *pPrimitive
            );

    private:
        GLTFTexturesAndBuffers* m_pGLTFTexturesAndBuffers;

        ResourceViewHeaps*  m_pResourceViewHeaps;
        DynamicBufferRing*  m_pDynamicBufferRing;
        StaticBufferPool*   m_pStaticBufferPool;

        std::vector<PBRMesh>     mMeshes;
        std::vector<PBRMaterial> mMaterialDatas;

        GLTFPBRPass::PerFrame mCBPerFrame;

        PBRMaterial mDefaultMaterial;

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
