#include "RHI/Vulkan/VKCommon/PCHVK.h"
#include "GLTFTexturesAndBuffersVK.h"
#include "Misc.h"
#include "GLTFHelpersVK.h"
#include "RHI/Vulkan/VKCommon/UploadHeapVK.h"
#include "ThreadPool.h"
#include "GLTFPBRMaterial.h"

using namespace LeoVultana_VK;

void GLTFTexturesAndBuffers::OnCreate(
    Device *pDevice,
    GLTFCommon *pGLTFCommon,
    UploadHeap* pUploadHeap,
    StaticBufferPool *pStaticBufferPool,
    DynamicBufferRing *pDynamicBufferRing)
{
    m_pDevice = pDevice;
    m_pGLTFCommon = pGLTFCommon;
    m_pUploadHeap = pUploadHeap;
    m_pStaticBufferPool = pStaticBufferPool;
    m_pDynamicBufferRing = pDynamicBufferRing;
}

void GLTFTexturesAndBuffers::LoadTextures(AsyncPool *pAsyncPool)
{
    // Load Texture and Create View
    if (m_pGLTFCommon->j3.find("images") != m_pGLTFCommon->j3.end())
    {
        m_pTextureNodes = &m_pGLTFCommon->j3["textures"];
        const json &images = m_pGLTFCommon->j3["images"];
        const json &materials = m_pGLTFCommon->j3["materials"];

        std::vector<Async *> taskQueue(images.size());

        mTextures.resize(images.size());
        mTextureViews.resize(images.size());
        for (int imageIndex = 0; imageIndex < images.size(); imageIndex++)
        {
            Texture* pTex = &mTextures[imageIndex];
            std::string filename = m_pGLTFCommon->mPath + images[imageIndex]["uri"].get<std::string>();

            ExecAsyncIfThereIsAPool(pAsyncPool, [imageIndex, pTex, this, filename, materials]()
            {
                bool useSRGB;
                float cutOff;
                GetSrgbAndCutOffOfImageGivenItsUse(imageIndex, materials, &useSRGB, &cutOff);
                bool result = pTex->InitFromFile(
                    m_pDevice, m_pUploadHeap,
                    filename.c_str(), useSRGB, 0, cutOff);
                assert(result != false);
                mTextures[imageIndex].CreateSRV(&mTextureViews[imageIndex]);
            });
        }
        LoadGeometry();

        if (pAsyncPool) pAsyncPool->Flush();
        m_pUploadHeap->FlushAndFinish();
    }
}

void GLTFTexturesAndBuffers::LoadGeometry()
{
    if (m_pGLTFCommon->j3.find("meshes") != m_pGLTFCommon->j3.end())
    {
        for (const json& mesh : m_pGLTFCommon->j3["meshes"])
        {
            for (const json& primitive : mesh["primitives"])
            {
                // Vertex Buffers
                for (const json& attributeID : primitive["attributes"])
                {
                    gltfAccessor vbAccessor;
                    m_pGLTFCommon->GetBufferDetails(attributeID, &vbAccessor);
                    VkDescriptorBufferInfo  vbv;
                    m_pStaticBufferPool->AllocateBuffer(vbAccessor.mCount, vbAccessor.mStride, vbAccessor.mData, &vbv);
                    mVertexBufferMap[attributeID] = vbv;
                }
                // Index Buffer
                int indexAccessor = primitive.value("indices", -1);
                if (indexAccessor >= 0)
                {
                    gltfAccessor ibAccessor;
                    m_pGLTFCommon->GetBufferDetails(indexAccessor, &ibAccessor);

                    VkDescriptorBufferInfo ibv;
                    if (ibAccessor.mStride == 1)
                    {
                        auto* pIndices = (unsigned short*)malloc(ibAccessor.mCount * (2 * ibAccessor.mStride));
                        for (int i = 0; i < ibAccessor.mCount; i++) pIndices[i] = ((unsigned char*)ibAccessor.mData)[i];
                        m_pStaticBufferPool->AllocateBuffer(ibAccessor.mCount, 2 * ibAccessor.mStride, ibAccessor.mData, &ibv);
                    }
                    else
                    {
                        m_pStaticBufferPool->AllocateBuffer(ibAccessor.mCount, ibAccessor.mStride, ibAccessor.mData, &ibv);
                    }
                    mIndexBufferMap[indexAccessor] = ibv;
                }
            }
        }
    }
}

void GLTFTexturesAndBuffers::OnDestroy()
{
    for (int i = 0; i < mTextures.size(); ++i)
    {
        vkDestroyImageView(m_pDevice->GetDevice(), mTextureViews[i], nullptr);
        mTextures[i].OnDestroy();
    }
}

void GLTFTexturesAndBuffers::CreateIndexBuffer(
    int indexBufferID,
    uint32_t *pNumIndices,
    VkIndexType *pIndexType,
    VkDescriptorBufferInfo *pIBV)
{
    gltfAccessor indexBuffer;
    m_pGLTFCommon->GetBufferDetails(indexBufferID, &indexBuffer);

    *pNumIndices = indexBuffer.mCount;
    *pIndexType = (indexBuffer.mStride == 4) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

    *pIBV = mIndexBufferMap[indexBufferID];
}

void GLTFTexturesAndBuffers::CreateGeometry(int indexBufferID, std::vector<int> &vertexBufferIDs, Geometry *pGeometry)
{
    CreateIndexBuffer(indexBufferID, &pGeometry->mNumIndices, &pGeometry->mIndexType, &pGeometry->mIBV);

    // load the rest of the buffers onto the GPU
    pGeometry->mVBV.resize(vertexBufferIDs.size());
    for (int i = 0; i < vertexBufferIDs.size(); i++)
    {
        pGeometry->mVBV[i] = mVertexBufferMap[vertexBufferIDs[i]];
    }
}

void GLTFTexturesAndBuffers::CreateGeometry(
    const json &primitive,
    const std::vector<std::string> requiredAttributes,
    std::vector<VkVertexInputAttributeDescription> &layout,
    DefineList &defines, Geometry *pGeometry)
{
    // Get Index buffer view
    gltfAccessor indexBuffer;
    int indexBufferId = primitive.value("indices", -1);
    CreateIndexBuffer(indexBufferId, &pGeometry->mNumIndices, &pGeometry->mIndexType, &pGeometry->mIBV);

    // Create vertex buffers and input layout
    int cnt = 0;
    layout.resize(requiredAttributes.size());
    pGeometry->mVBV.resize(requiredAttributes.size());
    const json &attributes = primitive.at("attributes");
    for (const auto& attrName : requiredAttributes)
    {
        // Get Vertex Buffer View
        const int attr = attributes.find(attrName).value();
        pGeometry->mVBV[cnt] = mVertexBufferMap[attr];

        // Let the compiler know we have this stream
        defines[std::string("ID_") + attrName] = std::to_string(cnt);

        const json &inAccessor = m_pGLTFCommon->m_pAccessors->at(attr);

        // Create Input Layout
        VkVertexInputAttributeDescription viAttributeDesc{};
        viAttributeDesc.location = (uint32_t)cnt;
        viAttributeDesc.format = GetFormat(inAccessor["type"], inAccessor["componentType"]);
        viAttributeDesc.offset = 0;
        viAttributeDesc.binding = cnt;
        layout[cnt] = viAttributeDesc;

        cnt++;
    }
}

VkImageView GLTFTexturesAndBuffers::GetTextureViewByID(int id)
{
    int tex = m_pTextureNodes->at(id)["source"];
    return mTextureViews[tex];
}

VkDescriptorBufferInfo *GLTFTexturesAndBuffers::GetSkinningMatricesBuffer(int skinIndex)
{
    auto it = mSkeletonMatricesBuffer.find(skinIndex);
    if (it == mSkeletonMatricesBuffer.end()) return nullptr;

    return &it->second;
}

void GLTFTexturesAndBuffers::SetSkinningMatricesForSkeletons()
{
    for (auto &t : m_pGLTFCommon->mWorldSpaceSkeletonMats)
    {
        std::vector<Matrix2>* matrices = &t.second;

        VkDescriptorBufferInfo perSkeleton{};
        Matrix2* cbPerSkeleton;
        auto size = (uint32_t)(matrices->size() * sizeof(Matrix2));
        m_pDynamicBufferRing->AllocateConstantBuffer(size, (void **)&cbPerSkeleton, &perSkeleton);
        memcpy(cbPerSkeleton, matrices->data(), size);
        mSkeletonMatricesBuffer[t.first] = perSkeleton;
    }
}

void GLTFTexturesAndBuffers::SetPerFrameConstants()
{
    PerFrame *cbPerFrame;
    m_pDynamicBufferRing->AllocateConstantBuffer(sizeof(PerFrame), (void **)&cbPerFrame, &mPerFrameConstants);
    *cbPerFrame = m_pGLTFCommon->mPerFrameData;
}
