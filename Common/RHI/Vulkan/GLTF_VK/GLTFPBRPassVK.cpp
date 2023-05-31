#include "RHI/Vulkan/VKCommon/PCHVK.h"
#include "GLTFPBRPassVK.h"
#include "Async.h"
#include "GLTFHelpersVK.h"
#include "RHI/Vulkan/VKCommon/HelperVK.h"
#include "RHI/Vulkan/VKCommon/ShaderCompilerHelperVK.h"
#include "RHI/Vulkan/VKCommon/ExtDebugUtilsVK.h"
#include "PostProcess/SkyDome.h"

using namespace LeoVultana_VK;

void PBRPrimitives::DrawPrimitive(
    VkCommandBuffer cmdBuffer,
    VkDescriptorBufferInfo perFrameDesc,
    VkDescriptorBufferInfo perObjectDesc,
    VkDescriptorBufferInfo *pPerSkeleton,
    bool bWireframe)
{
    // Bind indices and vertices using the right offsets into the buffer
    for (uint32_t i = 0; i < mGeometry.mVBV.size(); i++)
    {
        vkCmdBindVertexBuffers(cmdBuffer, i, 1, &mGeometry.mVBV[i].buffer, &mGeometry.mVBV[i].offset);
    }

    vkCmdBindIndexBuffer(cmdBuffer, mGeometry.mIBV.buffer, mGeometry.mIBV.offset, mGeometry.mIndexType);

    // Bind Descriptor sets
    VkDescriptorSet descritorSets[2] = { mUniformDescSet, m_pMaterial->mTextureDescSet };
    uint32_t descritorSetsCount = (m_pMaterial->mTextureCount == 0) ? 1 : 2;

    uint32_t uniformOffsets[3] = { (uint32_t)perFrameDesc.offset,  (uint32_t)perObjectDesc.offset, (pPerSkeleton) ? (uint32_t)pPerSkeleton->offset : 0 };
    uint32_t uniformOffsetsCount = (pPerSkeleton) ? 3 : 2;

    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        mPipelineLayout, 0,
        descritorSetsCount, descritorSets,
        uniformOffsetsCount, uniformOffsets);

    // Bind Pipeline
    if (bWireframe)
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineWireframe);
    else
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    // Draw
    vkCmdDrawIndexed(cmdBuffer, mGeometry.mNumIndices, 1, 0, 0, 0);
}

void GLTFPBRPass::OnCreate(
    Device *pDevice,
    UploadHeap *pUploadHeap,
    ResourceViewHeaps *pHeaps,
    DynamicBufferRing *pDynamicBufferRing,
    StaticBufferPool *pStaticBufferPool,
    GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
    SkyDome *pSkyDome,
    bool bUseSSAOMask,
    std::vector<VkImageView> &ShadowMapViewPool,
    GBufferRenderPass *pRenderPass,
    AsyncPool *pAsyncPool)
{
    m_pDevice = pDevice;
    m_pRenderPass = pRenderPass;
    m_pResourceViewHeaps = pHeaps;
    m_pStaticBufferPool = pStaticBufferPool;
    m_pDynamicBufferRing = pDynamicBufferRing;
    m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;

    //set bindings for the render targets
    DefineList rtDefines;
    m_pRenderPass->GetCompilerDefines(rtDefines);

    // Load BRDF look up table for the PBR shader
    mBRDFLutTexture.InitFromFile(pDevice, pUploadHeap, "BrdfLut.dds", false); // LUT images are stored as linear
    mBRDFLutTexture.CreateSRV(&mBRDFLutView);

    // Create Samplers
    //for pbr materials
    {
        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.minLod = 0;
        info.maxLod = 10000;
        info.maxAnisotropy = 1.0f;
        VK_CHECK_RESULT(vkCreateSampler(pDevice->GetDevice(), &info, nullptr, &mSamplerPBR));
    }
    // specular BRDF lut sampler
    {
        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.minLod = -1000;
        info.maxLod = 1000;
        info.maxAnisotropy = 1.0f;
        VK_CHECK_RESULT(vkCreateSampler(pDevice->GetDevice(), &info, nullptr, &mBRDFLutSampler));
    }
    // shadowmap sampler
    {
        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.compareEnable = VK_TRUE;
        info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        info.minLod = -1000;
        info.maxLod = 1000;
        info.maxAnisotropy = 1.0f;
        VK_CHECK_RESULT(vkCreateSampler(pDevice->GetDevice(), &info, nullptr, &mSamplerShadow));
    }

    // Create default material, this material will be used if none is assigned
    {
        SetDefaultMaterialParameters(&mDefaultMaterial.mPBRMaterialParameters);
        std::map<std::string, VkImageView> texturesBase;
        CreateDescriptorTableForMaterialTextures(&mDefaultMaterial, texturesBase, pSkyDome, ShadowMapViewPool, bUseSSAOMask);
    }

    // Load PBR 2.0 Materials
    const json& js = pGLTFTexturesAndBuffers->m_pGLTFCommon->j3;
    const json& materials = js["materials"];
    mMaterialDatas.resize(materials.size());
    for (uint32_t i = 0; i < materials.size(); i++)
    {
        PBRMaterial *tfMat = &mMaterialDatas[i];
        // Get PBR Material parameters and textures ID
        std::map<std::string, int> texturesIDs;
        ProcessMaterials(materials[i], &tfMat->mPBRMaterialParameters, texturesIDs);

        // translate texture IDs into textureViews
        std::map<std::string, VkImageView> textureBase;
        for (auto const& value : texturesIDs)
            textureBase[value.first] = m_pGLTFTexturesAndBuffers->GetTextureViewByID(value.second);

        CreateDescriptorTableForMaterialTextures(tfMat, textureBase, pSkyDome, ShadowMapViewPool, bUseSSAOMask);
    }

    // Load Meshes
    if (js.find("meshes") != js.end())
    {
        const json& meshes = js["meshes"];

        mMeshes.resize(meshes.size());
        for (uint32_t i = 0; i < meshes.size(); i++)
        {
            const json& primitves = meshes[i]["primitives"];

            // Loop through all the primitives (sets of triangles with a same material) and
            // 1) create an input layout for the geometry
            // 2) then take its material and create a Root descriptor
            // 3) With all the above, create a pipeline
            PBRMesh* tfMesh = &mMeshes[i];
            tfMesh->mPrimitives.resize(primitves.size());

            for (uint32_t p = 0; p < primitves.size(); p++)
            {
                const json& primitive = primitves[p];
                PBRPrimitives* pPrimitive = &tfMesh->mPrimitives[p];

                ExecAsyncIfThereIsAPool(pAsyncPool, [this, i, rtDefines, &primitive, pPrimitive, bUseSSAOMask]()
                {
                   // Set primitive's material
                   auto mat = primitive.find("material");
                   pPrimitive->m_pMaterial = (mat != primitive.end()) ? &mMaterialDatas[mat.value()] : &mDefaultMaterial;

                   // holds all #defines from materials, geometry and texture IDs, the VS & PS shaders need this to get the bindings and code paths
                   DefineList defines = pPrimitive->m_pMaterial->mPBRMaterialParameters.mDefines + rtDefines;

                   // make a list of all the attribute names our pass requires, in the case of PBR we need them all
                   std::vector<std::string> requiredAttributes;
                   for (auto const& it : primitive["attributes"].items()) requiredAttributes.push_back(it.key());

                   // create an input layout from the required attributes
                   // shader's can tell the slots from the #defines
                   std::vector<VkVertexInputAttributeDescription> viAttributeDesc;
                   m_pGLTFTexturesAndBuffers->CreateGeometry(primitive, requiredAttributes, viAttributeDesc, defines, &pPrimitive->mGeometry);

                   // Create descriptor and pipelines
                   int skinID = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i);
                   int inverseMatrixBufferSize = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetInverseBindMatricesBufferSizeByID(skinID);

                    CreateDescriptors(inverseMatrixBufferSize, &defines, pPrimitive, bUseSSAOMask);
                    CreatePipeline(viAttributeDesc, defines, pPrimitive);
                });
            }
        }
    }
}

void GLTFPBRPass::OnDestroy()
{
    for (uint32_t m = 0; m < mMeshes.size(); m++)
    {
        PBRMesh *pMesh = &mMeshes[m];
        for (uint32_t p = 0; p < pMesh->mPrimitives.size(); p++)
        {
            PBRPrimitives *pPrimitive = &pMesh->mPrimitives[p];
            vkDestroyPipeline(m_pDevice->GetDevice(), pPrimitive->mPipeline, nullptr);
            pPrimitive->mPipeline = VK_NULL_HANDLE;

            vkDestroyPipeline(m_pDevice->GetDevice(), pPrimitive->mPipelineWireframe, nullptr);
            pPrimitive->mPipelineWireframe = VK_NULL_HANDLE;

            vkDestroyPipelineLayout(m_pDevice->GetDevice(), pPrimitive->mPipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), pPrimitive->mUniformDescSetLayout, nullptr);
            m_pResourceViewHeaps->FreeDescriptor(pPrimitive->mUniformDescSet);
        }
    }

    for (int i = 0; i < mMaterialDatas.size(); i++)
    {
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), mMaterialDatas[i].mTextureDescSetLayout, nullptr);
        m_pResourceViewHeaps->FreeDescriptor(mMaterialDatas[i].mTextureDescSet);
    }

    //destroy default material
    vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), mDefaultMaterial.mTextureDescSetLayout, nullptr);
    m_pResourceViewHeaps->FreeDescriptor(mDefaultMaterial.mTextureDescSet);

    vkDestroySampler(m_pDevice->GetDevice(), mSamplerPBR, nullptr);
    vkDestroySampler(m_pDevice->GetDevice(), mSamplerShadow, nullptr);

    vkDestroyImageView(m_pDevice->GetDevice(), mBRDFLutView, nullptr);
    vkDestroySampler(m_pDevice->GetDevice(), mBRDFLutSampler, nullptr);
    mBRDFLutTexture.OnDestroy();
}

void GLTFPBRPass::BuildBatchLists(
    std::vector<BatchList> *pSolid,
    std::vector<BatchList> *pTransparent, bool bWireframe)
{
    // loop through nodes
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
        PBRMesh *pMesh = &mMeshes[pNode->meshIndex];
        for (uint32_t p = 0; p < pMesh->mPrimitives.size(); p++)
        {
            PBRPrimitives *pPrimitive = &pMesh->mPrimitives[p];

            if ((bWireframe && pPrimitive->mPipelineWireframe == VK_NULL_HANDLE) ||
                (!bWireframe && pPrimitive->mPipeline == VK_NULL_HANDLE))
                continue;

            // do frustrum culling
            gltfPrimitives boundingBox = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->mMeshes[pNode->meshIndex].m_pPrimitives[p];
            if (CameraFrustumToBoxCollision(mModelViewProj, boundingBox.mCenter, boundingBox.mRadius))
                continue;

            PBRMaterialParameters *pPbrParams = &pPrimitive->m_pMaterial->mPBRMaterialParameters;

            // Set per Object constants from material
            GLTFPBRPass::PerObject *cbPerObject;
            VkDescriptorBufferInfo perObjectDesc;
            m_pDynamicBufferRing->AllocateConstantBuffer(sizeof(GLTFPBRPass::PerObject), (void **)&cbPerObject, &perObjectDesc);
            cbPerObject->mCurrentWorld = pNodesMatrices[i].GetCurrent();
            cbPerObject->mPreviousWorld = pNodesMatrices[i].GetPrevious();
            cbPerObject->mPBRParams = pPbrParams->mParams;

            // compute depth for sorting
            math::Vector4 v = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->mMeshes[pNode->meshIndex].m_pPrimitives[p].mCenter;
            float depth = (mModelViewProj * v).getW();

            BatchList bcList{};
            bcList.mDepth = depth;
            bcList.m_pPrimitive = pPrimitive;
            bcList.mPerFrameDesc = m_pGLTFTexturesAndBuffers->mPerFrameConstants;
            bcList.mPerObjectDesc = perObjectDesc;
            bcList.m_pPerSkeleton = pPerSkeleton;

            // append primitive to list
            if (!pPbrParams->mBlending) pSolid->push_back(bcList);
            else pTransparent->push_back(bcList);
        }
    }
}

void GLTFPBRPass::DrawBatchList(
    VkCommandBuffer commandBuffer,
    std::vector<BatchList> *pBatchList, bool bWireframe)
{
    SetPerfMarkerBegin(commandBuffer, "gltfPBR");

    for (auto &t : *pBatchList)
    {
        t.m_pPrimitive->DrawPrimitive(
            commandBuffer,
            t.mPerFrameDesc,
            t.mPerObjectDesc,
            t.m_pPerSkeleton, bWireframe);
    }

    SetPerfMarkerEnd(commandBuffer);
}

void GLTFPBRPass::OnUpdateWindowSizeDependentResources(VkImageView SSAO)
{
    for (uint32_t i = 0; i < mMaterialDatas.size(); i++)
    {
        PBRMaterial* tfMat = &mMaterialDatas[i];
        DefineList define = tfMat->mPBRMaterialParameters.mDefines;
        auto ID = define.find("ID_SSAO");
        if (ID != define.end())
        {
            int index = std::stoi(ID->second);
            SetDescriptorSet(m_pDevice->GetDevice(), index, SSAO, &mSamplerPBR, tfMat->mTextureDescSet);
        }
    }
}

void GLTFPBRPass::CreateDescriptorTableForMaterialTextures(
    PBRMaterial *tfMat,
    std::map<std::string, VkImageView> &texturesBase,
    SkyDome *pSkyDome,
    std::vector<VkImageView> &ShadowMapViewPool,
    bool bUseSSAOMask)
{
    std::vector<uint32_t> descCounts;
    // count the number of textures to init bindings and descriptor
    {
        tfMat->mTextureCount = (int)texturesBase.size();
        for (int i = 0; i < texturesBase.size(); ++i) descCounts.push_back(1);

        if (pSkyDome)
        {
            tfMat->mTextureCount += 3;   // +3 because the skydome has a specular, diffusse and a BDRF LUT map
            descCounts.push_back(1);
            descCounts.push_back(1);
            descCounts.push_back(1);
        }

        if (bUseSSAOMask)
        {
            tfMat->mTextureCount += 1;
            descCounts.push_back(1);
        }

        //if (ShadowMapView != VK_NULL_HANDLE)
        if (!ShadowMapViewPool.empty())
        {
            assert(ShadowMapViewPool.size() <= MaxShadowInstances);
            tfMat->mTextureCount += (int)ShadowMapViewPool.size();//1;
            // this is an array of samplers/textures
            // We should set the exact number of descriptors to avoid validation errors
            descCounts.push_back(MaxShadowInstances);
        }
    }

    // Alloc a descriptor layout and init the descriptor set for the following textures
    // 1) all the textures of the PBR material (if any)
    // 2) the 3 textures used for IBL:
    //         - 1 BRDF LUT
    //         - 2 CubeMaps for the specular, diffuse
    // 3) SSAO texture
    // 4) the shadowMaps (array of MaxShadowInstances entries -- maximum)
    // for each entry we create a #define with that texture name that hold the id of the texture. That way the PS knows in what slot is each texture.
    {
        // allocate descriptor table for the textures
        m_pResourceViewHeaps->AllocDescriptor(
            descCounts, nullptr,
            &tfMat->mTextureDescSetLayout,
            &tfMat->mTextureDescSet);
        uint32_t cnt = 0;

        // 1) Create SRV for the PBR materials
        for (auto const& it : texturesBase)
        {
            tfMat->mPBRMaterialParameters.mDefines[std::string("ID_") + it.first] = std::to_string(cnt);
            SetDescriptorSet(m_pDevice->GetDevice(), cnt, it.second, &mSamplerPBR, tfMat->mTextureDescSet);
            cnt++;
        }
        // 2) 3 SRVs for the IBL probe
        if (pSkyDome)
        {
            tfMat->mPBRMaterialParameters.mDefines["ID_BRDFTexture"] = std::to_string(cnt);
            SetDescriptorSet(
                m_pDevice->GetDevice(),
                cnt, mBRDFLutView,
                &mBRDFLutSampler, tfMat->mTextureDescSet);
            cnt++;
            tfMat->mPBRMaterialParameters.mDefines["ID_DiffuseCube"] = std::to_string(cnt);
            pSkyDome->SetDescriptorDiffuse(cnt, tfMat->mTextureDescSet);
            cnt++;
            tfMat->mPBRMaterialParameters.mDefines["ID_SpecularCube"] = std::to_string(cnt);
            pSkyDome->SetDescriptorSpecular(cnt, tfMat->mTextureDescSet);
            cnt++;

            tfMat->mPBRMaterialParameters.mDefines["USE_IBL"] = "1";
        }
        // 3) SSAO Mask
        if (bUseSSAOMask)
        {
            tfMat->mPBRMaterialParameters.mDefines["ID_SSAO"] = std::to_string(cnt);
            cnt++;
        }
        // 4) Up to MaxShadowInstances SRVs for the shadowMaps
        if (!ShadowMapViewPool.empty())
        {
            tfMat->mPBRMaterialParameters.mDefines["ID_ShadowMap"] = std::to_string(cnt);
            SetDescriptorSet(
                m_pDevice->GetDevice(),
                cnt,
                descCounts[cnt],
                ShadowMapViewPool,
                &mSamplerShadow, tfMat->mTextureDescSet);
            cnt++;
        }

    }
}

void GLTFPBRPass::CreateDescriptors(
    int inverseMatrixBufferSize,
    DefineList *pAttributeDefines,
    PBRPrimitives *pPrimitive,
    bool bUseSSAOMask)
{
    // Creates descriptor set layout binding for the constant buffers
    std::vector<VkDescriptorSetLayoutBinding> descLayoutBinding(2);
    // Constant buffer 'per frame'
    descLayoutBinding[0].binding = 0;
    descLayoutBinding[0].descriptorCount = 1;
    descLayoutBinding[0].pImmutableSamplers = nullptr;
    descLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descLayoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    (*pAttributeDefines)["ID_PER_FRAME"] = std::to_string(descLayoutBinding[0].binding);

    // Constant buffer 'per object'
    descLayoutBinding[1].binding = 1;
    descLayoutBinding[1].descriptorCount = 1;
    descLayoutBinding[1].pImmutableSamplers = nullptr;
    descLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descLayoutBinding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    (*pAttributeDefines)["ID_PER_OBJECT"] = std::to_string(descLayoutBinding[1].binding);

    // Constant buffer holding the skinning matrices
    if (inverseMatrixBufferSize >= 0)
    {
        VkDescriptorSetLayoutBinding constBufferBinding{};
        // skinning matrices
        constBufferBinding.binding = 2;
        constBufferBinding.descriptorCount = 1;
        constBufferBinding.pImmutableSamplers = nullptr;
        constBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        constBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        (*pAttributeDefines)["ID_SKINNING_MATRICES"] = std::to_string(constBufferBinding.binding);

        descLayoutBinding.push_back(constBufferBinding);
    }
    m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(
        &descLayoutBinding,
        &pPrimitive->mUniformDescSetLayout,
        &pPrimitive->mUniformDescSet);

    // Init descriptors sets for the constant buffers
    m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(GLTFPBRPass::PerFrame), pPrimitive->mUniformDescSet);
    m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(GLTFPBRPass::PerObject), pPrimitive->mUniformDescSet);
    if (inverseMatrixBufferSize >= 0)
    {
        m_pDynamicBufferRing->SetDescriptorSet(2, (uint32_t)inverseMatrixBufferSize, pPrimitive->mUniformDescSet);
    }

    // Create the pipeline layout
    std::vector<VkDescriptorSetLayout> descSetLayout = { pPrimitive->mUniformDescSetLayout };
    if (pPrimitive->m_pMaterial->mTextureDescSetLayout != VK_NULL_HANDLE)
        descSetLayout.push_back(pPrimitive->m_pMaterial->mTextureDescSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.pNext = nullptr;
    pipelineLayoutCI.pushConstantRangeCount = 0;
    pipelineLayoutCI.pPushConstantRanges = nullptr;
    pipelineLayoutCI.setLayoutCount = (uint32_t)descSetLayout.size();
    pipelineLayoutCI.pSetLayouts = descSetLayout.data();
    VK_CHECK_RESULT(vkCreatePipelineLayout(
        m_pDevice->GetDevice(),
        &pipelineLayoutCI, nullptr,
        &pPrimitive->mPipelineLayout));

    SetResourceName(
        m_pDevice->GetDevice(),
        VK_OBJECT_TYPE_PIPELINE_LAYOUT,
        (uint64_t)pPrimitive->mPipelineLayout, "GLTFPBRPass PL");
}

void GLTFPBRPass::CreatePipeline(
    std::vector<VkVertexInputAttributeDescription> layout,
    const DefineList &defines,
    PBRPrimitives *pPrimitive)
{
    // Compile and create shaders
    VkPipelineShaderStageCreateInfo vertexShader = {}, fragmentShader = {};
    VKCompileFromFile(
        m_pDevice->GetDevice(),
        VK_SHADER_STAGE_VERTEX_BIT,
        "GLTFPBRPass-vert.glsl",
        "main", "",
        &defines, &vertexShader);
    VKCompileFromFile(
        m_pDevice->GetDevice(),
        VK_SHADER_STAGE_FRAGMENT_BIT,
        "GLTFPBRPass-frag.glsl",
        "main", "",
        &defines, &fragmentShader);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };

    // Create pipeline
    // vertex input state
    std::vector<VkVertexInputBindingDescription> viBindingDesc(layout.size());
    for (int i = 0; i < layout.size(); i++)
    {
        viBindingDesc[i].binding = layout[i].binding;
        viBindingDesc[i].stride = SizeOfFormat(layout[i].format);
        viBindingDesc[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }

    VkPipelineVertexInputStateCreateInfo viStateCI{};
    viStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    viStateCI.pNext = nullptr;
    viStateCI.flags = 0;
    viStateCI.vertexBindingDescriptionCount = (uint32_t)viBindingDesc.size();
    viStateCI.pVertexBindingDescriptions = viBindingDesc.data();
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
    rsStateCI.cullMode = pPrimitive->m_pMaterial->mPBRMaterialParameters.mDoubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
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
        VkPipelineColorBlendAttachmentState attachState = {};
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
        VkPipelineColorBlendAttachmentState attachState = {};
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
        VkPipelineColorBlendAttachmentState attachState = {};
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
    if (defines.Has("HAS_MOTION_VECTORS_RT"))
    {
        VkPipelineColorBlendAttachmentState attachState = {};
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
    VkPipelineMultisampleStateCreateInfo msState{};
    msState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msState.pNext = nullptr;
    msState.flags = 0;
    msState.pSampleMask = nullptr;
    msState.rasterizationSamples = m_pRenderPass->GetSampleCount();
    msState.sampleShadingEnable = VK_FALSE;
    msState.alphaToCoverageEnable = VK_FALSE;
    msState.alphaToOneEnable = VK_FALSE;
    msState.minSampleShading = 0.0;

    // create pipeline
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
    pipeline.pMultisampleState = &msState;
    pipeline.pDynamicState = &dynamicState;
    pipeline.pViewportState = &vpStateCI;
    pipeline.pDepthStencilState = &dsStateCI;
    pipeline.pStages = shaderStages.data();
    pipeline.stageCount = (uint32_t)shaderStages.size();
    pipeline.renderPass = m_pRenderPass->GetRenderPass();
    pipeline.subpass = 0;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        m_pDevice->GetDevice(),
        m_pDevice->GetPipelineCache(),
        1, &pipeline, nullptr,
        &pPrimitive->mPipeline));
    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)pPrimitive->mPipeline, "GLTFPBRPass P");

    // create wireframe pipeline
    rsStateCI.polygonMode = VK_POLYGON_MODE_LINE;
    rsStateCI.cullMode = VK_CULL_MODE_NONE;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        m_pDevice->GetDevice(),
        m_pDevice->GetPipelineCache(),
        1, &pipeline, nullptr,
        &pPrimitive->mPipelineWireframe));

    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)pPrimitive->mPipelineWireframe, "GLTFPBRPass Wireframe P");
}
