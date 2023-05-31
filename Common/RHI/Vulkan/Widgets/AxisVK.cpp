#include "RHI/Vulkan/VKCommon/PCHVK.h"
#include "AxisVK.h"
#include "RHI/Vulkan/VKCommon/DeviceVK.h"
#include "RHI/Vulkan/VKCommon/HelperVK.h"
#include "RHI/Vulkan/VKCommon/ShaderCompilerHelperVK.h"
#include "RHI/Vulkan/VKCommon/ExtDebugUtilsVK.h"

using namespace LeoVultana_VK;

void LeoVultana_VK::Axis::OnCreate(
    Device *pDevice,
    VkRenderPass renderPass,
    ResourceViewHeaps *pHeaps,
    DynamicBufferRing *pDynamicBufferRing,
    StaticBufferPool *pStaticBufferPool,
    VkSampleCountFlagBits sampleDescCount)
{
    m_pDevice = pDevice;
    m_pResourceViewHeaps = pHeaps;
    m_pDynamicBufferRing = pDynamicBufferRing;
    m_pStaticBufferPool = pStaticBufferPool;

    // Set indices
    short indices[] = {0, 1, 2, 3, 4, 5};
    mNumIndices = _countof(indices);
    mIndexType = VK_INDEX_TYPE_UINT16;

    void* pDest;
    m_pStaticBufferPool->AllocBuffer(mNumIndices, sizeof(short), &pDest, &mIBV);
    memcpy(pDest, indices, sizeof(short) * mNumIndices);

    // Set Vertex
    float vertices[] = {
        0,  0,  0,   1,0,0,
        1,  0,  0,   1,0,0,

        0,  0,  0,   0,1,0,
        0,  1,  0,   0,1,0,

        0,  0,  0,   0,0,1,
        0,  0,  1,   0,0,1,
    };
    m_pStaticBufferPool->AllocBuffer(6, 6 * sizeof(float), vertices, &mVBV);

    // Vertex Shader
    // the vertex shader
    static const char* vertexShader =
        "#version 400\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "layout (std140, binding = 0) uniform _cbPerObject\n"
        "{\n"
        "    mat4        u_mWorldViewProj;\n"
        "} cbPerObject;\n"
        "layout(location = 0) in vec3 position; \n"
        "layout(location = 1) in vec3 inColor; \n"
        "layout (location = 0) out vec3 outColor;\n"
        "void main() {\n"
        "   outColor = inColor;\n"
        "   gl_Position = cbPerObject.u_mWorldViewProj * vec4(position, 1.0f);\n"
        "}\n";

    // the pixel shader
    static const char* pixelShader =
        "#version 400\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "layout (location = 0) in vec3 inColor;\n"
        "layout (location = 0) out vec4 outColor;\n"
        "void main() {\n"
        "   outColor = vec4(inColor*5.0, 1.0);\n"
        "}";

    // Compiler Shaders
    DefineList attributeDefines;
    VkPipelineShaderStageCreateInfo vsCI{};
    VK_CHECK_RESULT(VKCompileFromString(
        pDevice->GetDevice(), SST_GLSL,
        VK_SHADER_STAGE_VERTEX_BIT, vertexShader,
        "main", "", &attributeDefines, &vsCI));
    VkPipelineShaderStageCreateInfo psCI{};
    VK_CHECK_RESULT(VKCompileFromString(
        pDevice->GetDevice(), SST_GLSL,
        VK_SHADER_STAGE_FRAGMENT_BIT, pixelShader,
        "main", "", &attributeDefines, &psCI));

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCIs = { vsCI, psCI };

    // Create Descriptor Set Layout and a Descriptor Set
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings(1);
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[0].pImmutableSamplers = nullptr;

    m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(
        &layoutBindings,
        &mDescriptorSetLayout, &mDescriptorSet);
    m_pDynamicBufferRing->SetDescriptorSet(0, sizeof(perObject), mDescriptorSet);

    // Create the pipeline layout using the descriptor set
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.pNext = nullptr;
    pipelineLayoutCI.pushConstantRangeCount = 0;
    pipelineLayoutCI.pPushConstantRanges = nullptr;
    pipelineLayoutCI.setLayoutCount = (uint32_t)1;
    pipelineLayoutCI.pSetLayouts = &mDescriptorSetLayout;
    VK_CHECK_RESULT(vkCreatePipelineLayout(pDevice->GetDevice(), &pipelineLayoutCI, nullptr, &mPipelineLayout));

    // Create the input attribute description / input layout
    VkVertexInputBindingDescription viBindingDesc{};
    viBindingDesc.binding = 0;
    viBindingDesc.stride = sizeof(float) * 6;
    viBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription viAttributeDesc[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    };

    VkPipelineVertexInputStateCreateInfo viStateCI{};
    viStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    viStateCI.pNext = nullptr;
    viStateCI.flags = 0;
    viStateCI.vertexBindingDescriptionCount = 1;
    viStateCI.pVertexBindingDescriptions = &viBindingDesc;
    viStateCI.vertexAttributeDescriptionCount = _countof(viAttributeDesc);
    viStateCI.pVertexAttributeDescriptions = viAttributeDesc;

    // Input Assembly State
    VkPipelineInputAssemblyStateCreateInfo iaStateCI{};
    iaStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaStateCI.pNext = nullptr;
    iaStateCI.flags = 0;
    iaStateCI.primitiveRestartEnable = VK_FALSE;
    iaStateCI.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    // Rasterizer State
    VkPipelineRasterizationStateCreateInfo rsStateCI{};
    rsStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rsStateCI.pNext = nullptr;
    rsStateCI.flags = 0;
    rsStateCI.polygonMode = VK_POLYGON_MODE_LINE;
    rsStateCI.cullMode = VK_CULL_MODE_NONE;
    rsStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rsStateCI.depthClampEnable = VK_FALSE;
    rsStateCI.rasterizerDiscardEnable = VK_FALSE;
    rsStateCI.depthBiasEnable = VK_FALSE;
    rsStateCI.depthBiasConstantFactor = 0;
    rsStateCI.depthBiasClamp = 0;
    rsStateCI.depthBiasSlopeFactor = 0;
    rsStateCI.lineWidth = 5.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState[1]{};
    colorBlendAttachmentState[0].colorWriteMask = 0xf;
    colorBlendAttachmentState[0].blendEnable = VK_TRUE;
    colorBlendAttachmentState[0].alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState[0].colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    // Color Blend
    VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
    colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCI.flags = 0;
    colorBlendStateCI.pNext = nullptr;
    colorBlendStateCI.attachmentCount = 1;
    colorBlendStateCI.pAttachments = colorBlendAttachmentState;
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

    // Viewport State
    VkPipelineViewportStateCreateInfo viewportStateCI{};
    viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCI.pNext = nullptr;
    viewportStateCI.flags = 0;
    viewportStateCI.viewportCount = 1;
    viewportStateCI.scissorCount = 1;
    viewportStateCI.pScissors = nullptr;
    viewportStateCI.pViewports = nullptr;

    // Depth Stencil State
    VkPipelineDepthStencilStateCreateInfo dsStateCI{};
    dsStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dsStateCI.pNext = nullptr;
    dsStateCI.flags = 0;
    dsStateCI.depthTestEnable = true;
    dsStateCI.depthWriteEnable = true;
    dsStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    dsStateCI.depthBoundsTestEnable = VK_FALSE;
    dsStateCI.stencilTestEnable = VK_FALSE;
    dsStateCI.back.failOp = VK_STENCIL_OP_KEEP;
    dsStateCI.back.passOp = VK_STENCIL_OP_KEEP;
    dsStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
    dsStateCI.back.compareMask = 0;
    dsStateCI.back.reference = 0;
    dsStateCI.back.depthFailOp = VK_STENCIL_OP_KEEP;
    dsStateCI.back.writeMask = 0;
    dsStateCI.minDepthBounds = 0;
    dsStateCI.maxDepthBounds = 0;
    dsStateCI.stencilTestEnable = VK_FALSE;
    dsStateCI.front = dsStateCI.back;

    // Multi Sample State
    VkPipelineMultisampleStateCreateInfo msStateCI{};
    msStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msStateCI.pNext = nullptr;
    msStateCI.flags = 0;
    msStateCI.pSampleMask = nullptr;
    msStateCI.rasterizationSamples = sampleDescCount;
    msStateCI.sampleShadingEnable = VK_FALSE;
    msStateCI.alphaToCoverageEnable = VK_FALSE;
    msStateCI.alphaToOneEnable = VK_FALSE;
    msStateCI.minSampleShading = 0.0;

    // create pipeline
    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.pNext = nullptr;
    pipelineCI.layout = mPipelineLayout;
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
    pipelineCI.pViewportState = &viewportStateCI;
    pipelineCI.pDepthStencilState = &dsStateCI;
    pipelineCI.pStages = shaderStageCIs.data();
    pipelineCI.stageCount = (uint32_t)shaderStageCIs.size();
    pipelineCI.renderPass = renderPass;
    pipelineCI.subpass = 0;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        pDevice->GetDevice(),
        pDevice->GetPipelineCache(),
        1, &pipelineCI,
        nullptr, &mPipeline));

    SetResourceName(pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)mPipeline, "Axis P");
}

void LeoVultana_VK::Axis::OnDestroy()
{
    vkDestroyPipeline(m_pDevice->GetDevice(), mPipeline, nullptr);
    vkDestroyPipelineLayout(m_pDevice->GetDevice(), mPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), mDescriptorSetLayout, nullptr);
    m_pResourceViewHeaps->FreeDescriptor(mDescriptorSet);
}

void LeoVultana_VK::Axis::Draw(
    VkCommandBuffer cmdBuffer,
    const math::Matrix4 &worldMatrix,
    const math::Matrix4 &axisMatrix)
{
    if (mPipeline == VK_NULL_HANDLE) return;

    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mVBV.buffer, &mVBV.offset);
    vkCmdBindIndexBuffer(cmdBuffer, mIBV.buffer, mIBV.offset, mIndexType);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    VkDescriptorSet descriptorSet[1] = { mDescriptorSet };

    // Set per Object Constants
    perObject* cbPerObject;
    VkDescriptorBufferInfo perObjectDesc;
    m_pDynamicBufferRing->AllocateConstantBuffer(sizeof(perObject), (void**)&cbPerObject, &perObjectDesc);
    cbPerObject->mWorldViewProj = axisMatrix * worldMatrix;

    uint32_t uniformOffsets[1] = { (uint32_t)perObjectDesc.offset };
    vkCmdBindDescriptorSets(
        cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        mPipelineLayout, 0, 1,
        descriptorSet, 1, uniformOffsets);

    vkCmdDrawIndexed(cmdBuffer, mNumIndices, 1, 0, 0, 0);
}
