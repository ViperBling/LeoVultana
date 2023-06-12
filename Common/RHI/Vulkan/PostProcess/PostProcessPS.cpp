#include "RHI/Vulkan/VKCommon/PCHVK.h"
#include "PostProcessPS.h"
#include "RHI/Vulkan/VKCommon/ExtDebugUtilsVK.h"
#include "RHI/Vulkan/VKCommon/HelperVK.h"
#include "RHI/Vulkan/VKCommon/ShaderCompilerHelperVK.h"
#include "RHI/Vulkan/VKCommon/UploadHeapVK.h"
#include "RHI/Vulkan/VKCommon/TextureVK.h"
#include "ThreadPool.h"

using namespace LeoVultana_VK;

void PostProcessPS::OnCreate(
    Device *pDevice,
    VkRenderPass renderPass,
    const std::string &shaderFilename,
    const std::string &shaderEntryPoint,
    const std::string &shaderCompilerParams,
    StaticBufferPool *pStaticBufferPool,
    DynamicBufferRing *pDynamicBufferRing,
    VkDescriptorSetLayout descriptorSetLayout,
    VkPipelineColorBlendStateCreateInfo *pBlendDesc,
    VkSampleCountFlagBits sampleDescCount)
{
    m_pDevice = pDevice;

    // Create the vertex shader
    static const char* vertexShader =
        "static const float4 FullScreenVertsPos[3] = { float4(-1, 1, 1, 1), float4(3, 1, 1, 1), float4(-1, -3, 1, 1) };\
        static const float2 FullScreenVertsUVs[3] = { float2(0, 0), float2(2, 0), float2(0, 2) };\
        struct VERTEX_OUT\
        {\
            float2 vTexture : TEXCOORD;\
            float4 vPosition : SV_POSITION;\
        };\
        VERTEX_OUT mainVS(uint vertexId : SV_VertexID)\
        {\
            VERTEX_OUT Output;\
            Output.vPosition = FullScreenVertsPos[vertexId];\
            Output.vTexture = FullScreenVertsUVs[vertexId];\
            return Output;\
        }";

    // Compile Shaders
    DefineList attributeDefines;

    VkPipelineShaderStageCreateInfo mVS;
#ifdef _DEBUG
    std::string CompileFlagsVS("-T vs_6_0 -Zi -Od");
#else
    std::string CompileFlagsVS("-T vs_6_0");
#endif // _DEBUG
    VK_CHECK_RESULT(VKCompileFromString(
        m_pDevice->GetDevice(),SST_HLSL,
        VK_SHADER_STAGE_VERTEX_BIT, vertexShader,
        "mainVS", CompileFlagsVS.c_str(),
        &attributeDefines, &mVS));

    mFragmentShaderName = shaderEntryPoint;
    VkPipelineShaderStageCreateInfo mPS;
    VK_CHECK_RESULT(VKCompileFromFile(
        m_pDevice->GetDevice(),
        VK_SHADER_STAGE_FRAGMENT_BIT,
        shaderFilename.c_str(),
        mFragmentShaderName.c_str(),
        shaderCompilerParams.c_str(),
        &attributeDefines, &mPS));

    mShaderStages.clear();
    mShaderStages.push_back(mVS);
    mShaderStages.push_back(mPS);

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.pNext = nullptr;
    pipelineLayoutCI.pushConstantRangeCount = 0;
    pipelineLayoutCI.pPushConstantRanges = nullptr;
    pipelineLayoutCI.setLayoutCount = 1;
    pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
    VK_CHECK_RESULT(vkCreatePipelineLayout(pDevice->GetDevice(), &pipelineLayoutCI, nullptr, &mPipelineLayout));

    UpdatePipeline(renderPass, pBlendDesc, sampleDescCount);
}

void PostProcessPS::OnDestroy()
{
    if (mPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_pDevice->GetDevice(), mPipeline, nullptr);
        mPipeline = VK_NULL_HANDLE;
    }
    if (mPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_pDevice->GetDevice(), mPipelineLayout, nullptr);
        mPipelineLayout = VK_NULL_HANDLE;
    }
}

void PostProcessPS::UpdatePipeline(
    VkRenderPass renderPass,
    VkPipelineColorBlendStateCreateInfo *pBlendDesc,
    VkSampleCountFlagBits sampleDescCount)
{
    if (renderPass == VK_NULL_HANDLE) return;

    if (mPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_pDevice->GetDevice(), mPipeline, nullptr);
        mPipeline = VK_NULL_HANDLE;
    }

    // input assembly state and layout
    VkPipelineVertexInputStateCreateInfo viStateCI{};
    viStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    viStateCI.pNext = nullptr;
    viStateCI.flags = 0;
    viStateCI.vertexBindingDescriptionCount = 0;
    viStateCI.pVertexBindingDescriptions = nullptr;
    viStateCI.vertexAttributeDescriptionCount = 0;
    viStateCI.pVertexAttributeDescriptions = nullptr;

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
    rsStateCI.cullMode = VK_CULL_MODE_NONE;
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
    attachStateCI[0].blendEnable = VK_FALSE;
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
    colorBlendStateCI.attachmentCount = 1;
    colorBlendStateCI.pAttachments = attachStateCI;
    colorBlendStateCI.logicOpEnable = VK_FALSE;
    colorBlendStateCI.logicOp = VK_LOGIC_OP_NO_OP;
    colorBlendStateCI.blendConstants[0] = 1.0f;
    colorBlendStateCI.blendConstants[1] = 1.0f;
    colorBlendStateCI.blendConstants[2] = 1.0f;
    colorBlendStateCI.blendConstants[3] = 1.0f;

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS
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
    dsStateCI.depthTestEnable = VK_TRUE;
    dsStateCI.depthWriteEnable = VK_FALSE;
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

    // multi sample state
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
    VkGraphicsPipelineCreateInfo pipeline = {};
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.pNext = nullptr;
    pipeline.layout = mPipelineLayout;
    pipeline.basePipelineHandle = VK_NULL_HANDLE;
    pipeline.basePipelineIndex = 0;
    pipeline.flags = 0;
    pipeline.pVertexInputState = &viStateCI;
    pipeline.pInputAssemblyState = &iaStateCI;
    pipeline.pRasterizationState = &rsStateCI;
    pipeline.pColorBlendState = (pBlendDesc == nullptr) ? &colorBlendStateCI : pBlendDesc;
    pipeline.pTessellationState = nullptr;
    pipeline.pMultisampleState = &msStateCI;
    pipeline.pDynamicState = &dynamicState;
    pipeline.pViewportState = &vpStateCI;
    pipeline.pDepthStencilState = &dsStateCI;
    pipeline.pStages = mShaderStages.data();
    pipeline.stageCount = (uint32_t)mShaderStages.size();
    pipeline.renderPass = renderPass;
    pipeline.subpass = 0;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_pDevice->GetDevice(), m_pDevice->GetPipelineCache(), 1, &pipeline, nullptr, &mPipeline))

    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)mPipeline, "PostProcess P");
}

void PostProcessPS::Draw(
    VkCommandBuffer cmdBuffer,
    VkDescriptorBufferInfo *pConstantBuffer,
    VkDescriptorSet descriptorSet)
{
    if (mPipeline == VK_NULL_HANDLE) return;

    // Bind Descriptor sets
    int numUniformOffsets = 0;
    uint32_t uniformOffset = 0;
    if (pConstantBuffer != nullptr && pConstantBuffer->buffer != VK_NULL_HANDLE)
    {
        numUniformOffsets = 1;
        uniformOffset = (uint32_t)pConstantBuffer->offset;
    }
    VkDescriptorSet descritorSets[1] = { descriptorSet };
    vkCmdBindDescriptorSets(
        cmdBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        mPipelineLayout, 0, 1,
        descritorSets,
        numUniformOffsets, &uniformOffset);

    // Bind Pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    // Draw
    vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
}
