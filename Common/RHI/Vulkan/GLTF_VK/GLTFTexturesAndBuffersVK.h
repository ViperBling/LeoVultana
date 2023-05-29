#pragma once

#include "GLTFCommon.h"
#include "TextureVK.h"
#include "ShaderCompiler.h"
#include "StaticBufferPoolVK.h"
#include "DynamicBufferRingVK.h"

namespace LeoVultana_VK
{
    struct Geometry
    {
        VkIndexType mIndexType;
        uint32_t mNumIndices;
        VkDescriptorBufferInfo mIBV;
        std::vector<VkDescriptorBufferInfo> mVBV;
    };

    class GLTFTexturesAndBuffers
    {
    public:
        void OnCreate(
            Device *pDevice,
            GLTFCommon *pGLTFCommon,
            UploadHeap* pUploadHeap,
            StaticBufferPool *pStaticBufferPool,
            DynamicBufferRing *pDynamicBufferRing);
        void LoadTextures(AsyncPool *pAsyncPool = nullptr);
        void LoadGeometry();
        void OnDestroy();

        void CreateIndexBuffer(
            int indexBufferID,
            uint32_t *pNumIndices,
            VkIndexType *pIndexType,
            VkDescriptorBufferInfo *pIBV);
        void CreateGeometry(int indexBufferID, std::vector<int> &vertexBufferIDs, Geometry *pGeometry);
        void CreateGeometry(
            const json &primitive,
            const std::vector<std::string> requiredAttributes,
            std::vector<VkVertexInputAttributeDescription> &layout,
            DefineList &defines, Geometry *pGeometry);

        VkImageView GetTextureViewByID(int id);

        VkDescriptorBufferInfo *GetSkinningMatricesBuffer(int skinIndex);
        void SetSkinningMatricesForSkeletons();
        void SetPerFrameConstants();
    public:
        GLTFCommon*             m_pGLTFCommon;
        VkDescriptorBufferInfo  mPerFrameConstants;

    private:
        Device*         m_pDevice;
        UploadHeap*     m_pUploadHeap;

        const json*     m_pTextureNodes;

        std::vector<Texture>        mTextures;
        std::vector<VkImageView>    mTextureViews;

        std::map<int, VkDescriptorBufferInfo> mSkeletonMatricesBuffer;

        StaticBufferPool*   m_pStaticBufferPool;
        DynamicBufferRing*  m_pDynamicBufferRing;

        // Maps GLTF ids into views
        std::map<int, VkDescriptorBufferInfo> mVertexBufferMap;
        std::map<int, VkDescriptorBufferInfo> mIndexBufferMap;
    };
}