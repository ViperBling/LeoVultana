#include "PCHVK.h"
#include "GLTFDepthPassVK.h"
#include "GLTFHelpersVK.h"
#include "ShaderCompilerHelperVK.h"
#include "ExtDebugUtilsVK.h"
#include "ResourceViewHeapsVK.h"
#include "HelperVK.h"
#include "Async.h"
#include "GLTFPBRMaterial.h"

using namespace LeoVultana_VK;

void GLTFDepthPass::OnCreate(
    Device *pDevice,
    VkRenderPass renderPass,
    UploadHeap *pUploadHeap,
    ResourceViewHeaps *pHeaps,
    DynamicBufferRing *pDynamicBufferRing,
    StaticBufferPool *pStaticBufferPool,
    GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
    AsyncPool *pAsyncPool)
{
    m_pDevice = pDevice;
    mRenderPass = renderPass;
    m_pResourceViewHeaps = pHeaps;
    m_pStaticBufferPool = pStaticBufferPool;
    m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;
    m_pDynamicBufferRing = pDynamicBufferRing;

    const json& js = pGLTFTexturesAndBuffers->m_pGLTFCommon->j3;

    // Create default material
    mDefaultMaterial.mTextureCount = 0;
    mDefaultMaterial.mDescSet = VK_NULL_HANDLE;
    mDefaultMaterial.mDescSetLayout = VK_NULL_HANDLE;
    mDefaultMaterial.mDoubleSided = false;

    // Create static sampler in case there is transparency
    VkSamplerCreateInfo samplerCI{};
    samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCI.magFilter = VK_FILTER_LINEAR;
    samplerCI.minFilter = VK_FILTER_LINEAR;
    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.minLod = -1000;
    samplerCI.maxLod = 1000;
    samplerCI.maxAnisotropy = 1.0f;
    VK_CHECK_RESULT(vkCreateSampler(pDevice->GetDevice(), &samplerCI, nullptr, &mSampler))

    // Create materials (in a depth pass materials are still needed to handle none opaque textures)
    if (js.find("materials") != js.end())
    {
        const json& materials = js["materials"];

        mMaterialDatas.resize(materials.size());
        for (uint32_t i = 0; i < materials.size(); i++)
        {
            const json &material = materials[i];

            DepthMaterial *gltfMat = &mMaterialDatas[i];

            // Load material constants. This is a depth pass, and we are only interested in the mask texture
            gltfMat->mDoubleSided = GetElementBoolean(material, "doubleSided", false);
            std::string alphaMode = GetElementString(material, "alphaMode", "OPAQUE");
            gltfMat->mDefines["DEF_alphaMode_" + alphaMode] = std::to_string(1);

            // If transparent use the baseColorTexture for alpha
            if (alphaMode == "MASK")
            {
                gltfMat->mDefines["DEF_alphaCutoff"] = std::to_string(GetElementFloat(material, "alphaCutoff", 0.5));

                auto pbrMetallicRoughnessIt = material.find("pbrMetallicRoughness");
                if (pbrMetallicRoughnessIt != material.end())
                {
                    const json &pbrMetallicRoughness = pbrMetallicRoughnessIt.value();

                    int id = GetElementInt(pbrMetallicRoughness, "baseColorTexture/index", -1);
                    if (id >= 0)
                    {
                        gltfMat->mDefines["MATERIAL_METALLICROUGHNESS"] = "1";

                        // allocate descriptor table for the texture
                        gltfMat->mTextureCount = 1;
                        gltfMat->mDefines["ID_baseColorTexture"] = "0";
                        gltfMat->mDefines["ID_baseTexCoord"] = std::to_string(GetElementInt(pbrMetallicRoughness, "baseColorTexture/texCoord", 0));
                        m_pResourceViewHeaps->AllocDescriptor(gltfMat->mTextureCount, &mSampler, &gltfMat->mDescSetLayout, &gltfMat->mDescSet);
                        VkImageView textureView = pGLTFTexturesAndBuffers->GetTextureViewByID(id);
                        SetDescriptorSet(m_pDevice->GetDevice(), 0, textureView, &mSampler, gltfMat->mDescSet);
                    }
                }
            }
        }
    }

    // Load meshes
    if (js.find("meshes") != js.end())
    {
        const json& meshes = js["meshes"];
        mMeshes.resize(meshes.size());
        for (uint32_t i = 0; i < meshes.size(); ++i)
        {
            DepthMesh* gltfMesh = &mMeshes[i];
            const json& primitives = meshes[i]["primitives"];
            gltfMesh->mPrimitives.resize(primitives.size());

            for (uint32_t p = 0; p < primitives.size(); ++p)
            {
                const json& primitive = primitives[p];
                DepthPrimitives* pPrimitive = &gltfMesh->mPrimitives[p];

                ExecAsyncIfThereIsAPool(pAsyncPool, [this, i, &primitive, pPrimitive]()
                {
                    // Set Material
                    auto mat = primitive.find("material");
                    if (mat != primitive.end()) pPrimitive->m_pMaterial = &mMaterialDatas[mat.value()];
                    else pPrimitive->m_pMaterial = &mDefaultMaterial;

                    // make a list of all the attribute names our pass requires, in the case of a depth pass we only need the position and a few other things.
                    std::vector<std::string> requiredAttributes;
                    for (auto const & it : primitive["attributes"].items())
                    {
                        const std::string semanticName = it.key();
                        if (semanticName == "POSITION" ||
                            semanticName.substr(0, 7) == "WEIGHTS" ||
                            semanticName.substr(0, 6) == "JOINTS" ||
                            DoesMaterialUseSemantic(pPrimitive->m_pMaterial->mDefines, semanticName))
                        {
                            requiredAttributes.push_back(semanticName);
                        }
                    }
                    // holds all the #defines from materials, geometry and texture IDs, the VS & PS shaders need this to get the bindings and code paths
                    DefineList defines = pPrimitive->m_pMaterial->mDefines;

                    // create an input layout from the required attributes
                    // shader's can tell the slots from the #defines
                    std::vector<VkVertexInputAttributeDescription> viAttributeDescs;
                    m_pGLTFTexturesAndBuffers->CreateGeometry(primitive, requiredAttributes, viAttributeDescs, defines, &pPrimitive->mGeometry);

                    // Create pipeline
                    int skinID = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->FindMeshSkinId(i);
                    int inverseMatrixBufferSize = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->GetInverseBindMatricesBufferSizeByID(skinID);
                    CreateDescriptors(inverseMatrixBufferSize, &defines, pPrimitive);
                    CreatePipeline(viAttributeDescs, defines, pPrimitive);
                });
            }
        }
    }
}

void GLTFDepthPass::OnDestroy()
{
    for (uint32_t m = 0; m < mMeshes.size(); m++)
    {
        DepthMesh *pMesh = &mMeshes[m];
        for (uint32_t p = 0; p < pMesh->mPrimitives.size(); p++)
        {
            DepthPrimitives *pPrimitive = &pMesh->mPrimitives[p];
            vkDestroyPipeline(m_pDevice->GetDevice(), pPrimitive->mPipeline, nullptr);
            pPrimitive->mPipeline = VK_NULL_HANDLE;
            vkDestroyPipelineLayout(m_pDevice->GetDevice(), pPrimitive->mPipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), pPrimitive->mDescSetLayout, nullptr);
            m_pResourceViewHeaps->FreeDescriptor(pPrimitive->mDescSet);
        }
    }
    for (int i = 0; i < mMaterialDatas.size(); i++)
    {
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), mMaterialDatas[i].mDescSetLayout, nullptr);
        m_pResourceViewHeaps->FreeDescriptor(mMaterialDatas[i].mDescSet);
    }
    vkDestroySampler(m_pDevice->GetDevice(), mSampler, nullptr);
}

GLTFDepthPass::PerFrame *GLTFDepthPass::SetPerFrameConstants()
{
    return nullptr;
}

void GLTFDepthPass::Draw(VkCommandBuffer cmdBuffer)
{

}

void GLTFDepthPass::CreateDescriptors(
    int inverseMatrixBufferSize,
    DefineList *pAttributeDefines,
    DepthPrimitives *pPrimitives)
{
    std::vector<VkDescriptorSetLayoutBinding> descLayoutBindings(2);
    descLayoutBindings[0].binding = 0;
    descLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descLayoutBindings[0].descriptorCount = 1;
    descLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descLayoutBindings[0].pImmutableSamplers = nullptr;
    (*pAttributeDefines)["ID_PER_FRAME"] = std::to_string(descLayoutBindings[0].binding);

    descLayoutBindings[1].binding = 1;
    descLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descLayoutBindings[1].descriptorCount = 1;
    descLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descLayoutBindings[1].pImmutableSamplers = nullptr;
    (*pAttributeDefines)["ID_PER_OBJECT"] = std::to_string(descLayoutBindings[1].binding);

    if (inverseMatrixBufferSize > 0)
    {
        VkDescriptorSetLayoutBinding b;

        // skinning matrices
        b.binding = 2;
        b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        b.descriptorCount = 1;
        b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        b.pImmutableSamplers = nullptr;
        (*pAttributeDefines)["ID_SKINNING_MATRICES"] = std::to_string(b.binding);

        descLayoutBindings.push_back(b);
    }
    m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&descLayoutBindings, &pPrimitives->mDescSetLayout, &pPrimitives->mDescSet);

    // set descriptors entries
    m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(mPerFrame), pPrimitives->mDescSet);
    m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(mPerObject), pPrimitives->mDescSet);

    if (inverseMatrixBufferSize > 0)
    {
        m_pDynamicBufferRing->SetDescriptorSet(2, (uint32_t)inverseMatrixBufferSize, pPrimitives->mDescSet);
    }

    std::vector<VkDescriptorSetLayout> descriptorSetLayout = { pPrimitives->mDescSetLayout };
    if (pPrimitives->m_pMaterial->mDescSetLayout != VK_NULL_HANDLE)
        descriptorSetLayout.push_back(pPrimitives->m_pMaterial->mDescSetLayout);

    /////////////////////////////////////////////
    // Create a PSO description

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext = nullptr;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayout.size();
    pPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout.data();

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_pDevice->GetDevice(), &pPipelineLayoutCreateInfo, nullptr, &pPrimitives->mPipelineLayout));
    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)pPrimitives->mPipelineLayout, "GlTFDepthPass PL");
}

void GLTFDepthPass::CreatePipeline(
    std::vector<VkVertexInputAttributeDescription> layout,
    const DefineList &defineList,
    DepthPrimitives *pPrimitives)
{
    /////////////////////////////////////////////
    // Compile and create shaders
    VkPipelineShaderStageCreateInfo vertexShader, fragmentShader{};
    VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT, "GLTFDepthPass-vert.glsl", "main", "", &defineList, &vertexShader);
    VKCompileFromFile(m_pDevice->GetDevice(), VK_SHADER_STAGE_FRAGMENT_BIT, "GLTFDepthPass-frag.glsl", "main", "", &defineList, &fragmentShader);
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertexShader, fragmentShader };

    /////////////////////////////////////////////
    // Create a Pipeline 

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
    rsStateCI.cullMode = pPrimitives->m_pMaterial->mDoubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;;
    rsStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rsStateCI.depthClampEnable = VK_FALSE;
    rsStateCI.rasterizerDiscardEnable = VK_FALSE;
    rsStateCI.depthBiasEnable = VK_FALSE;
    rsStateCI.depthBiasConstantFactor = 0;
    rsStateCI.depthBiasClamp = 0;
    rsStateCI.depthBiasSlopeFactor = 0;
    rsStateCI.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState attachStateCI[1]{};
    attachStateCI[0].colorWriteMask = 0xf;
    attachStateCI[0].blendEnable = VK_TRUE;
    attachStateCI[0].alphaBlendOp = VK_BLEND_OP_ADD;
    attachStateCI[0].colorBlendOp = VK_BLEND_OP_ADD;
    attachStateCI[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    attachStateCI[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachStateCI[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachStateCI[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    // Color blend state
    VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
    colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCI.flags = 0;
    colorBlendStateCI.pNext = nullptr;
    colorBlendStateCI.attachmentCount = 0; //set to 1 when transparency
    colorBlendStateCI.pAttachments = attachStateCI;
    colorBlendStateCI.logicOpEnable = VK_FALSE;
    colorBlendStateCI.logicOp = VK_LOGIC_OP_NO_OP;
    colorBlendStateCI.blendConstants[0] = 1.0f;
    colorBlendStateCI.blendConstants[1] = 1.0f;
    colorBlendStateCI.blendConstants[2] = 1.0f;
    colorBlendStateCI.blendConstants[3] = 1.0f;

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
    msStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msStateCI.sampleShadingEnable = VK_FALSE;
    msStateCI.alphaToCoverageEnable = VK_FALSE;
    msStateCI.alphaToOneEnable = VK_FALSE;
    msStateCI.minSampleShading = 0.0;

    // create pipeline
    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.pNext = nullptr;
    pipelineCI.layout = pPrimitives->mPipelineLayout;
    pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCI.basePipelineIndex = 0;
    pipelineCI.flags = 0;
    pipelineCI.pVertexInputState = &viStateCI;
    pipelineCI.pInputAssemblyState = &iaStateCI;
    pipelineCI.pRasterizationState = &rsStateCI;
    pipelineCI.pColorBlendState = &colorBlendStateCI;
    pipelineCI.pTessellationState = nullptr;
    pipelineCI.pMultisampleState = &msStateCI;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.pViewportState = &vpStateCI;
    pipelineCI.pDepthStencilState = &dsStateCI;
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.stageCount = (uint32_t)shaderStages.size();
    pipelineCI.renderPass = mRenderPass;
    pipelineCI.subpass = 0;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        m_pDevice->GetDevice(),
        m_pDevice->GetPipelineCache(),
        1, &pipelineCI,
        nullptr, &pPrimitives->mPipeline));
    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)pPrimitives->mPipeline, "GlTFDepthPass P");
}
