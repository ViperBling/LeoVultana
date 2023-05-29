#pragma once

#include "GLTFTexturesAndBuffersVK.h"

namespace LeoVultana_VK
{
    struct DepthMaterial
    {
        int mTextureCount = 0;
        VkDescriptorSet mDescSet{};
        VkDescriptorSetLayout mDescSetLayout{};

        DefineList mDefines;
        bool mDoubleSided = false;
    };

    struct DepthPrimitives
    {
        Geometry mGeometry;
        DepthMaterial* m_pMaterial = nullptr;

        VkPipeline mPipeline{};
        VkPipelineLayout mPipelineLayout{};

        VkDescriptorSet mDescSet{};
        VkDescriptorSetLayout mDescSetLayout{};
    };

    struct DepthMesh
    {
        std::vector<DepthPrimitives> mPrimitives;
    };

    class GLTFDepthPass
    {
    public:
        struct PerFrame
        {
            math::Matrix4 mViewProj;
        } mPerFrame;
        struct PerObject
        {
            math::Matrix4 mWorld;
        } mPerObject;

    public:
        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            UploadHeap* pUploadHeap,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
            AsyncPool *pAsyncPool = nullptr);

        void OnDestroy();
        GLTFDepthPass::PerFrame* SetPerFrameConstants();
        void Draw(VkCommandBuffer cmdBuffer);

    private:
        void CreateDescriptors(int inverseMatrixBufferSize, DefineList* pAttributeDefines, DepthPrimitives* pPrimitives);
        void CreatePipeline(std::vector<VkVertexInputAttributeDescription> layout, const DefineList& defineList, DepthPrimitives* pPrimitives);

    private:
        Device*                 m_pDevice;
        VkRenderPass            mRenderPass{};
        VkSampler               mSampler{};
        VkDescriptorBufferInfo  mPerFrameDesc;

        ResourceViewHeaps*      m_pResourceViewHeaps;
        DynamicBufferRing*      m_pDynamicBufferRing;
        StaticBufferPool*       m_pStaticBufferPool;

        std::vector<DepthMesh>      mMeshes;
        std::vector<DepthMaterial>  mMaterialDatas;
        DepthMaterial               mDefaultMaterial;

        GLTFTexturesAndBuffers*     m_pGLTFTexturesAndBuffers;
    };
}