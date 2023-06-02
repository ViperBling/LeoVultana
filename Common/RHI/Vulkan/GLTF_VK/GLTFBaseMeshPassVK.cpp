#include "RHI/Vulkan/VKCommon/PCHVK.h"
#include "GLTFBaseMeshPassVK.h"
#include "Async.h"
#include "RHI/Vulkan/GLTF_VK/GLTFHelpersVK.h"
#include "RHI/Vulkan/VKCommon/HelperVK.h"
#include "RHI/Vulkan/VKCommon/ShaderCompilerHelperVK.h"
#include "RHI/Vulkan/VKCommon/ExtDebugUtilsVK.h"

using namespace LeoVultana_VK;


void BasePassPrimitives::DrawPrimitive(
    VkCommandBuffer cmdBuffer,
    VkDescriptorBufferInfo perFrameDesc,
    VkDescriptorBufferInfo perObjectDesc)
{
    for (uint32_t i = 0; i < mGeometry.mVBV.size(); i++)
    {
        vkCmdBindVertexBuffers(cmdBuffer, i, 1, &mGeometry.mVBV[i].buffer, &mGeometry.mVBV[i].offset);
    }
    vkCmdBindIndexBuffer(cmdBuffer, mGeometry.mIBV.buffer, mGeometry.mIBV.offset, mGeometry.mIndexType);

    // DescSet
    VkDescriptorSet descSets[2] = { mUniformDescSet, m_pMats->mTexturesDescSet };
    uint32_t descSetCount = (m_pMats->mTextureCount == 0) ? 1 : 2;
    uint32_t uniformOffsets[3] = { (uint32_t)perFrameDesc.offset, (uint32_t)perObjectDesc.offset, 0};
    uint32_t uniformOffsetsCount = 2;
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, descSetCount, descSets, uniformOffsetsCount, uniformOffsets);

    // pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    // Draw
    vkCmdDrawIndexed(cmdBuffer, mGeometry.mNumIndices, 1, 0, 0, 0);
}

void GLTFBaseMeshPass::OnCreate(
    Device *pDevice, UploadHeap *pUploadHeap,
    ResourceViewHeaps *pResourceViewHeap,
    DynamicBufferRing *pDynamicBufferRing,
    StaticBufferPool *pStaticBufferPool,
    GLTFTexturesAndBuffers *pGLTFTexAndBuffers,
    std::vector<VkImageView>& ShadowMapViewPool,
    GBufferRenderPass *pRenderPass,AsyncPool *pAsyncPool)
{
    m_pDevice = pDevice;
    m_pRenderPass = pRenderPass;
    m_pResourceViewHeaps = pResourceViewHeap;
    m_pDynamicBufferRing = pDynamicBufferRing;
    m_pStaticBufferPool = pStaticBufferPool;
    m_pGLTFTexturesAndBuffers = pGLTFTexAndBuffers;

    DefineList rtDefines;
    m_pRenderPass->GetCompilerDefines(rtDefines);

    // Material
    VkSamplerCreateInfo matSamplerCI{};
    matSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    matSamplerCI.magFilter = VK_FILTER_LINEAR;
    matSamplerCI.minFilter = VK_FILTER_LINEAR;
    matSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    matSamplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    matSamplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    matSamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    matSamplerCI.minLod = 0;
    matSamplerCI.maxLod = 10000;
    matSamplerCI.maxAnisotropy = 1.0f;
    VK_CHECK_RESULT(vkCreateSampler(pDevice->GetDevice(), &matSamplerCI, nullptr, &mSamplerMat))

    VkSamplerCreateInfo shadowSamplerCI{};
    shadowSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    shadowSamplerCI.magFilter = VK_FILTER_LINEAR;
    shadowSamplerCI.minFilter = VK_FILTER_LINEAR;
    shadowSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    shadowSamplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shadowSamplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shadowSamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    shadowSamplerCI.compareEnable = VK_TRUE;
    shadowSamplerCI.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    shadowSamplerCI.minLod = -1000;
    shadowSamplerCI.maxLod = 1000;
    shadowSamplerCI.maxAnisotropy = 1.0f;
    VK_CHECK_RESULT(vkCreateSampler(pDevice->GetDevice(), &shadowSamplerCI, nullptr, &mSamplerShadow))

    // Default Mat
    SetDefaultMaterialParameters(&mDefaultMaterial.mBasePassMatParams);
    std::map<std::string, VkImageView> textureBase;
    CreateDescTableForMaterialTextures(&mDefaultMaterial, textureBase, ShadowMapViewPool);

    // Load GLTF Mat
    const json& js = pGLTFTexAndBuffers->m_pGLTFCommon->j3;

    const json& materials = js["materials"];
    mMaterialDatas.resize(materials.size());
    for (uint32_t i = 0; i < materials.size(); i++)
    {
        BasePassMaterial* gltfMat = &mMaterialDatas[i];
        std::map<std::string, int> textureIDs;
        ProcessMaterials(materials[i], &gltfMat->mBasePassMatParams, textureIDs);

        std::map<std::string, VkImageView> textureViews;
        for (auto const& value : textureIDs)
            textureViews[value.first] = m_pGLTFTexturesAndBuffers->GetTextureViewByID(value.second);
    }

    // Load mesh
    if (js.find("meshes") != js.end())
    {
        const json& meshes = js["meshes"];

        mMeshes.resize(meshes.size());

        for (uint32_t i = 0; i < meshes.size(); i++)
        {
            const json& primitives = meshes[i]["primitives"];

            BasePassMesh* gltfMesh = &mMeshes[i];
            gltfMesh->mPrimitives.resize(primitives.size());

            for (uint32_t p = 0; p < primitives.size(); p++)
            {
                const json& primitive = primitives[p];
                BasePassPrimitives* pPrimitive = &gltfMesh->mPrimitives[p];

                ExecAsyncIfThereIsAPool(pAsyncPool, [this, i, rtDefines, &primitive, pPrimitive]()
                {
                    auto mat = primitive.find("material");
                    pPrimitive->m_pMats = (mat != primitive.end()) ? & mMaterialDatas[mat.value()] : &mDefaultMaterial;

                    DefineList defines = pPrimitive->m_pMats->mBasePassMatParams.mDefines + rtDefines;

                    std::vector<std::string> requiredAttributes;
                    for (auto const& it : primitive["attributes"].items()) requiredAttributes.push_back(it.key());

                    std::vector<VkVertexInputAttributeDescription> inputLayout;
                    m_pGLTFTexturesAndBuffers->CreateGeometry(primitive, requiredAttributes, inputLayout, defines, &pPrimitive->mGeometry);

                    // Create pipeline
                    int skinID = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i);
                    int inverseMatBufferSize = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetInverseBindMatricesBufferSizeByID(skinID);
                    CreateDescriptors(inverseMatBufferSize, &defines, pPrimitive);
                    CreatePipeline(inputLayout, defines, pPrimitive);
                });
            }
        }
    }

}

void GLTFBaseMeshPass::OnDestroy()
{
    for (uint32_t m = 0; m < mMeshes.size(); m++)
    {
        BasePassMesh *pMesh = &mMeshes[m];
        for (uint32_t p = 0; p < pMesh->mPrimitives.size(); p++)
        {
            BasePassPrimitives *pPrimitive = &pMesh->mPrimitives[p];
            vkDestroyPipeline(m_pDevice->GetDevice(), pPrimitive->mPipeline, nullptr);
            pPrimitive->mPipeline = VK_NULL_HANDLE;

            vkDestroyPipelineLayout(m_pDevice->GetDevice(), pPrimitive->mPipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), pPrimitive->mUniformDescSetLayout, nullptr);
            m_pResourceViewHeaps->FreeDescriptor(pPrimitive->mUniformDescSet);
        }
    }

    for (int i = 0; i < mMaterialDatas.size(); i++)
    {
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), mMaterialDatas[i].mTextureDescSetLayout, nullptr);
        m_pResourceViewHeaps->FreeDescriptor(mMaterialDatas[i].mTexturesDescSet);
    }

    //destroy default material
    vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), mDefaultMaterial.mTextureDescSetLayout, nullptr);
    m_pResourceViewHeaps->FreeDescriptor(mDefaultMaterial.mTexturesDescSet);

    vkDestroySampler(m_pDevice->GetDevice(), mSamplerMat, nullptr);
    vkDestroySampler(m_pDevice->GetDevice(), mSamplerShadow, nullptr);
}

void GLTFBaseMeshPass::BuildBatchList(
    std::vector<BatchList> *pSolid,
    std::vector<BatchList> *pTransparent)
{
    // loop through nodes
    //
    std::vector<gltfNode> *pNodes = &m_pGLTFTexturesAndBuffers->m_pGLTFCommon->mNodes;
    Matrix2 *pNodesMatrices = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->mWorldSpaceMats.data();

    for (uint32_t i = 0; i < pNodes->size(); i++)
    {
        gltfNode *pNode = &pNodes->at(i);
        if ((pNode == nullptr) || (pNode->meshIndex < 0))
            continue;

        // skinning matrices constant buffer
        VkDescriptorBufferInfo *pPerSkeleton = m_pGLTFTexturesAndBuffers->GetSkinningMatricesBuffer(pNode->skinIndex);

        math::Matrix4 mModelViewProj =  m_pGLTFTexturesAndBuffers->m_pGLTFCommon->mPerFrameData.mCameraCurrViewProj * pNodesMatrices[i].GetCurrent();

        // loop through primitives
        BasePassMesh *pMesh = &mMeshes[pNode->meshIndex];
        for (uint32_t p = 0; p < pMesh->mPrimitives.size(); p++)
        {
            BasePassPrimitives *pPrimitive = &pMesh->mPrimitives[p];

            if ( pPrimitive->mPipeline == VK_NULL_HANDLE)
                continue;

            // do frustrum culling
            //
            gltfPrimitives boundingBox = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->mMeshes[pNode->meshIndex].m_pPrimitives[p];
            if (CameraFrustumToBoxCollision(mModelViewProj, boundingBox.mCenter, boundingBox.mRadius))
                continue;

            PBRMaterialParameters *pPbrParams = &pPrimitive->m_pMats->mBasePassMatParams;

            // Set per Object constants from material
            GLTFBaseMeshPass::PerObjectData *cbPerObject;
            VkDescriptorBufferInfo perObjectDesc;
            m_pDynamicBufferRing->AllocateConstantBuffer(sizeof(GLTFBaseMeshPass::PerObjectData), (void **)&cbPerObject, &perObjectDesc);
            cbPerObject->mCurrentWorld = pNodesMatrices[i].GetCurrent();
            cbPerObject->mPreviousWorld = pNodesMatrices[i].GetPrevious();
            cbPerObject->mPBRParams = pPbrParams->mParams;

            // compute depth for sorting
            math::Vector4 v = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->mMeshes[pNode->meshIndex].m_pPrimitives[p].mCenter;
            float depth = (mModelViewProj * v).getW();

            BatchList t{};
            t.mDepth = depth;
            t.mPrimitives = pPrimitive;
            t.mPerFrameDesc = m_pGLTFTexturesAndBuffers->mPerFrameConstants;
            t.mPerObjectDesc = perObjectDesc;

            // append primitive to list
            if (!pPbrParams->mBlending) pSolid->push_back(t);
            else pTransparent->push_back(t);
        }
    }
}

void GLTFBaseMeshPass::DrawBatchList(VkCommandBuffer cmdBuffer, std::vector<BatchList> *pBatchList)
{
    SetPerfMarkerBegin(cmdBuffer, "GLTFBasePass");
    for (auto& t : *pBatchList)
    {
        t.mPrimitives->DrawPrimitive(cmdBuffer, t.mPerFrameDesc, t.mPerObjectDesc);
    }
    SetPerfMarkerEnd(cmdBuffer);
}

void GLTFBaseMeshPass::CreateDescTableForMaterialTextures(
    BasePassMaterial *gltfMat,
    std::map<std::string, VkImageView> &textureBase,
    std::vector<VkImageView> &ShadowMapViewPool)
{
    std::vector<uint32_t> descriptorCounts;
    // count the number of textures to init bindings and descriptor
    {
        gltfMat->mTextureCount = (int)textureBase.size();
        for (int i = 0; i < textureBase.size(); ++i)
        {
            descriptorCounts.push_back(1);
        }
        
        //if (ShadowMapView != VK_NULL_HANDLE)
        if (!ShadowMapViewPool.empty())
        {
            assert(ShadowMapViewPool.size() <= MaxShadowInstances);
            gltfMat->mTextureCount += (int)ShadowMapViewPool.size();//1;
            // this is an array of samplers/textures
            // We should set the exact number of descriptors to avoid validation errors
            descriptorCounts.push_back(MaxShadowInstances);
        }
    }

    // Alloc a descriptor layout and init the descriptor set for the following textures 
    // 1) all the textures of the PBR material (if any)
    // 2) the shadowmaps (array of MaxShadowInstances entries -- maximum)
    // for each entry we create a #define with that texture name that hold the id of the texture. That way the PS knows in what slot is each texture.      
    {
        // allocate descriptor table for the textures
        m_pResourceViewHeaps->AllocDescriptor(descriptorCounts, nullptr, &gltfMat->mTextureDescSetLayout, &gltfMat->mTexturesDescSet);

        uint32_t cnt = 0;

        // 1) create SRV for the materials
        for (auto const &it : textureBase)
        {
            gltfMat->mBasePassMatParams.mDefines[std::string("ID_") + it.first] = std::to_string(cnt);
            SetDescriptorSet(m_pDevice->GetDevice(), cnt, it.second, &mSamplerMat, gltfMat->mTexturesDescSet);
            cnt++;
        }

        // 4) Up to MaxShadowInstances SRVs for the shadowmaps
        if (!ShadowMapViewPool.empty())
        {
            gltfMat->mBasePassMatParams.mDefines["ID_shadowMap"] = std::to_string(cnt);

            SetDescriptorSet(m_pDevice->GetDevice(), cnt, descriptorCounts[cnt], ShadowMapViewPool, &mSamplerShadow, gltfMat->mTexturesDescSet);
            cnt++;
        }
    }
}

void GLTFBaseMeshPass::CreateDescriptors(
    int inverseMatrixBufferSize,
    DefineList *pAttributeDefines,
    BasePassPrimitives *pPrimitive)
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBinds(2);

    // Constant Buffer Perframe
    layoutBinds[0].binding = 0;
    layoutBinds[0].descriptorCount = 1;
    layoutBinds[0].pImmutableSamplers = nullptr;
    layoutBinds[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layoutBinds[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    (*pAttributeDefines)["ID_PER_FRAME"] = std::to_string(layoutBinds[0].binding);

    layoutBinds[1].binding = 1;
    layoutBinds[1].descriptorCount = 1;
    layoutBinds[1].pImmutableSamplers = nullptr;
    layoutBinds[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layoutBinds[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    (*pAttributeDefines)["ID_PER_OBJECT"] = std::to_string(layoutBinds[1].binding);

    if (inverseMatrixBufferSize >= 0)
    {
        VkDescriptorSetLayoutBinding bind;
        // skinning matrices
        bind.binding = 2;
        bind.descriptorCount = 1;
        bind.pImmutableSamplers = nullptr;
        bind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        (*pAttributeDefines)["ID_SKINNING_MATRICES"] = std::to_string(bind.binding);

        layoutBinds.push_back(bind);
    }

    m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBinds, &pPrimitive->mUniformDescSetLayout, &pPrimitive->mUniformDescSet);

    // Init Desc Set for constant Buffer
    m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(PerFrame), pPrimitive->mUniformDescSet);
    m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(GLTFBaseMeshPass::PerObjectData), pPrimitive->mUniformDescSet);

    if (inverseMatrixBufferSize >= 0) m_pDynamicBufferRing->SetDescriptorSet(2, (uint32_t)inverseMatrixBufferSize, pPrimitive->mUniformDescSet);

    // Pipeline Layout
    std::vector<VkDescriptorSetLayout> descSetLayout = { pPrimitive->mUniformDescSetLayout };
    if (pPrimitive->m_pMats->mTextureDescSetLayout != VK_NULL_HANDLE)
        descSetLayout.push_back(pPrimitive->m_pMats->mTextureDescSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.pNext = nullptr;
    pipelineLayoutCI.pushConstantRangeCount = 0;
    pipelineLayoutCI.pPushConstantRanges = nullptr;
    pipelineLayoutCI.setLayoutCount = (uint32_t)descSetLayout.size();
    pipelineLayoutCI.pSetLayouts = descSetLayout.data();

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_pDevice->GetDevice(), &pipelineLayoutCI, nullptr, &pPrimitive->mPipelineLayout));
    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)pPrimitive->mPipelineLayout, "GLTFBassPass PL");
}

void GLTFBaseMeshPass::CreatePipeline(
    std::vector<VkVertexInputAttributeDescription> layout,
    const DefineList &defines,
    BasePassPrimitives *pPrimitive)
{
    // Compile and create shaders
    VkPipelineShaderStageCreateInfo vertexShader = {}, fragmentShader = {};
    VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT, "GLTFBasePass-vert.glsl", "main", "", &defines, &vertexShader);
    VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, "GLTFBasePass-frag.glsl", "main", "", &defines, &fragmentShader);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };

    // Create pipeline

    // vertex input state
    std::vector<VkVertexInputBindingDescription> viBindDesc(layout.size());
    for (int i = 0; i < layout.size(); i++)
    {
        viBindDesc[i].binding = layout[i].binding;
        viBindDesc[i].stride = SizeOfFormat(layout[i].format);
        viBindDesc[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }

    VkPipelineVertexInputStateCreateInfo viStateCI{};
    viStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    viStateCI.pNext = nullptr;
    viStateCI.flags = 0;
    viStateCI.vertexBindingDescriptionCount = (uint32_t)viBindDesc.size();
    viStateCI.pVertexBindingDescriptions = viBindDesc.data();
    viStateCI.vertexAttributeDescriptionCount = (uint32_t)layout.size();
    viStateCI.pVertexAttributeDescriptions = layout.data();

    // input assembly state

    VkPipelineInputAssemblyStateCreateInfo iaStateCI{};
    iaStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaStateCI.pNext = nullptr;
    iaStateCI.flags = 0;
    iaStateCI.primitiveRestartEnable = VK_FALSE;
    iaStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // rasterizer state

    VkPipelineRasterizationStateCreateInfo rsStateCI{};
    rsStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rsStateCI.pNext = nullptr;
    rsStateCI.flags = 0;
    rsStateCI.polygonMode = VK_POLYGON_MODE_FILL;
    rsStateCI.cullMode = pPrimitive->m_pMats->mBasePassMatParams.mDoubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
    rsStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rsStateCI.depthClampEnable = VK_FALSE;
    rsStateCI.rasterizerDiscardEnable = VK_FALSE;
    rsStateCI.depthBiasEnable = VK_FALSE;
    rsStateCI.depthBiasConstantFactor = 0;
    rsStateCI.depthBiasClamp = 0;
    rsStateCI.depthBiasSlopeFactor = 0;
    rsStateCI.lineWidth = 1.0f;


    std::vector<VkPipelineColorBlendAttachmentState> cbAttachStates;
    if (defines.Has("HAS_FORWARD_RT"))
    {
        VkPipelineColorBlendAttachmentState attachState{};
        attachState.colorWriteMask = 0xf;
        attachState.blendEnable = (defines.Has("DEF_alphaMode_BLEND"));
        attachState.colorBlendOp = VK_BLEND_OP_ADD;
        attachState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachState.alphaBlendOp = VK_BLEND_OP_ADD;
        attachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cbAttachStates.push_back(attachState);
    }
    if (defines.Has("HAS_SPECULAR_ROUGHNESS_RT"))
    {
        VkPipelineColorBlendAttachmentState attachState{};
        attachState.colorWriteMask = 0xf;
        attachState.blendEnable = VK_FALSE;
        attachState.alphaBlendOp = VK_BLEND_OP_ADD;
        attachState.colorBlendOp = VK_BLEND_OP_ADD;
        attachState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cbAttachStates.push_back(attachState);
    }
    if (defines.Has("HAS_DIFFUSE_RT"))
    {
        VkPipelineColorBlendAttachmentState attachState{};
        attachState.colorWriteMask = 0xf;
        attachState.blendEnable = VK_FALSE;
        attachState.alphaBlendOp = VK_BLEND_OP_ADD;
        attachState.colorBlendOp = VK_BLEND_OP_ADD;
        attachState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cbAttachStates.push_back(attachState);
    }
    if (defines.Has("HAS_NORMALS_RT"))
    {
        VkPipelineColorBlendAttachmentState attachState{};
        attachState.colorWriteMask = 0xf;
        attachState.blendEnable = VK_FALSE;
        attachState.alphaBlendOp = VK_BLEND_OP_ADD;
        attachState.colorBlendOp = VK_BLEND_OP_ADD;
        attachState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cbAttachStates.push_back(attachState);
    }

    // Color blend state

    VkPipelineColorBlendStateCreateInfo cbStateCI{};
    cbStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbStateCI.flags = 0;
    cbStateCI.pNext = nullptr;
    cbStateCI.attachmentCount = static_cast<uint32_t>(cbAttachStates.size());
    cbStateCI.pAttachments = cbAttachStates.data();
    cbStateCI.logicOpEnable = VK_FALSE;
    cbStateCI.logicOp = VK_LOGIC_OP_NO_OP;
    cbStateCI.blendConstants[0] = 1.0f;
    cbStateCI.blendConstants[1] = 1.0f;
    cbStateCI.blendConstants[2] = 1.0f;
    cbStateCI.blendConstants[3] = 1.0f;

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

    // view port state
    VkPipelineViewportStateCreateInfo vpStateCI{};
    vpStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpStateCI.pNext = nullptr;
    vpStateCI.flags = 0;
    vpStateCI.viewportCount = 1;
    vpStateCI.scissorCount = 1;
    vpStateCI.pScissors = nullptr;
    vpStateCI.pViewports = nullptr;

    // depth stencil state
    VkPipelineDepthStencilStateCreateInfo dsStateCI{};
    dsStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dsStateCI.pNext = nullptr;
    dsStateCI.flags = 0;
    dsStateCI.depthTestEnable = true;
    dsStateCI.depthWriteEnable = true;
    dsStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    dsStateCI.back.failOp = VK_STENCIL_OP_KEEP;
    dsStateCI.back.passOp = VK_STENCIL_OP_KEEP;
    dsStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
    dsStateCI.back.compareMask = 0;
    dsStateCI.back.reference = 0;
    dsStateCI.back.depthFailOp = VK_STENCIL_OP_KEEP;
    dsStateCI.back.writeMask = 0;
    dsStateCI.depthBoundsTestEnable = VK_FALSE;
    dsStateCI.minDepthBounds = 0;
    dsStateCI.maxDepthBounds = 0;
    dsStateCI.stencilTestEnable = VK_FALSE;
    dsStateCI.front = dsStateCI.back;

    // multi sample state
    VkPipelineMultisampleStateCreateInfo msStateCI{};
    msStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msStateCI.pNext = nullptr;
    msStateCI.flags = 0;
    msStateCI.pSampleMask = nullptr;
    msStateCI.rasterizationSamples = m_pRenderPass->GetSampleCount();
    msStateCI.sampleShadingEnable = VK_FALSE;
    msStateCI.alphaToCoverageEnable = VK_FALSE;
    msStateCI.alphaToOneEnable = VK_FALSE;
    msStateCI.minSampleShading = 0.0;

    // create pipeline 
    //
    VkGraphicsPipelineCreateInfo pipeline = {};
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.pNext = nullptr;
    pipeline.layout = pPrimitive->mPipelineLayout;
    pipeline.basePipelineHandle = VK_NULL_HANDLE;
    pipeline.basePipelineIndex = 0;
    pipeline.flags = 0;
    pipeline.pVertexInputState = &viStateCI;
    pipeline.pInputAssemblyState = &iaStateCI;
    pipeline.pRasterizationState = &rsStateCI;
    pipeline.pColorBlendState = &cbStateCI;
    pipeline.pTessellationState = nullptr;
    pipeline.pMultisampleState = &msStateCI;
    pipeline.pDynamicState = &dynamicState;
    pipeline.pViewportState = &vpStateCI;
    pipeline.pDepthStencilState = &dsStateCI;
    pipeline.pStages = shaderStages.data();
    pipeline.stageCount = (uint32_t)shaderStages.size();
    pipeline.renderPass = m_pRenderPass->GetRenderPass();
    pipeline.subpass = 0;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_pDevice->GetDevice(), m_pDevice->GetPipelineCache(), 1, &pipeline, nullptr, &pPrimitive->mPipeline))
    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)pPrimitive->mPipeline, "GLTFBasePass P");
}
