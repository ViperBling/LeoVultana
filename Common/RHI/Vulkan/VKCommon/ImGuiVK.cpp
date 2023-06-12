#include "PCHVK.h"
#include "ImGuiVK.h"
#include "ShaderCompiler.h"
#include "ShaderCompilerHelperVK.h"
#include "ExtDebugUtilsVK.h"
#include "HelperVK.h"

#include <shellscalingapi.h>

namespace LeoVultana_VK
{
    // Data
    static HWND g_hWnd = nullptr;

    struct VERTEX_CONSTANT_BUFFER
    {
        float mvp[4][4];
    };

    void ImGUI::OnCreate(
        Device *pDevice,
        VkRenderPass renderPass,
        UploadHeap *pUploadHeap,
        DynamicBufferRing *pConstantBufferRing,
        float fontSize)
    {
        m_pConstBuf = pConstantBufferRing;
        m_pDevice = pDevice;
        mCurrentDescriptorIndex = 0;

        // Get UI texture
        ImGuiIO& io = ImGui::GetIO();

        // Fixup font size based on scale factor
        DEVICE_SCALE_FACTOR scaleFactor = SCALE_100_PERCENT;
        float textScale = (float)scaleFactor / 100.f;
        ImFontConfig fontCfg;
        fontCfg.SizePixels = fontSize * textScale;
        io.Fonts->AddFontDefault(&fontCfg);

        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        size_t upload_size = width * height * 4 * sizeof(char);

        // Create the texture object
        {
            VkImageCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.format = VK_FORMAT_R8G8B8A8_UNORM;
            info.extent.width = width;
            info.extent.height = height;
            info.extent.depth = 1;
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VmaAllocationCreateInfo imageAllocCreateInfo = {};
            imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
            imageAllocCreateInfo.pUserData = (void*)"ImGUI tex";
            VmaAllocationInfo gpuImageAllocInfo = {};
            VK_CHECK_RESULT(vmaCreateImage(m_pDevice->GetAllocator(), &info, &imageAllocCreateInfo, &mTexture2D, &mImageAlloc, &gpuImageAllocInfo))
            SetResourceName(pDevice->GetDevice(), VK_OBJECT_TYPE_IMAGE, (uint64_t)mTexture2D, (const char*)imageAllocCreateInfo.pUserData);
        }

        // Create the Image View
        {
            VkImageViewCreateInfo imageViewCI{};
            imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCI.image = mTexture2D;
            imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCI.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCI.subresourceRange.levelCount = 1;
            imageViewCI.subresourceRange.layerCount = 1;
            VK_CHECK_RESULT(vkCreateImageView(pDevice->GetDevice(), &imageViewCI, nullptr, &mTextureSRV));

            SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)mTextureSRV, "ImGUI tex");
        }

        // Tell ImGUI what the image view is
        io.Fonts->TexID = (void *)mTextureSRV;

        // Allocate memory int upload heap and copy the texture into it
        char *ptr = (char*)pUploadHeap->SubAllocate(upload_size, 512);

        memcpy(ptr, pixels, width * height * 4);

        // Copy from upload heap into the vid mem image
        {
            VkImageMemoryBarrier copyBarrier[1] = {};
            copyBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            copyBarrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            copyBarrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            copyBarrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            copyBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            copyBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            copyBarrier[0].image = mTexture2D;
            copyBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyBarrier[0].subresourceRange.levelCount = 1;
            copyBarrier[0].subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(
                pUploadHeap->GetCommandList(),
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                nullptr, 0,
                nullptr, 1, copyBarrier);

            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = width;
            region.imageExtent.height = height;
            region.imageExtent.depth = 1;
            vkCmdCopyBufferToImage(
                pUploadHeap->GetCommandList(), 
                pUploadHeap->GetResource(), mTexture2D,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            VkImageMemoryBarrier useBarrier[1] = {};
            useBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            useBarrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            useBarrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            useBarrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            useBarrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            useBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            useBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            useBarrier[0].image = mTexture2D;
            useBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            useBarrier[0].subresourceRange.levelCount = 1;
            useBarrier[0].subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(
                pUploadHeap->GetCommandList(),
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                0, 0, nullptr, 0, 
                nullptr, 1, useBarrier);
        }

        // Kick off the upload
        pUploadHeap->FlushAndFinish();

        // Create sampler
        {
            VkSamplerCreateInfo samplerCI{};
            samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerCI.magFilter = VK_FILTER_NEAREST;
            samplerCI.minFilter = VK_FILTER_NEAREST;
            samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCI.minLod = -1000;
            samplerCI.maxLod = 1000;
            samplerCI.maxAnisotropy = 1.0f;
            VK_CHECK_RESULT(vkCreateSampler(pDevice->GetDevice(), &samplerCI, nullptr, &mSampler));
        }

        // Vertex shader
        const char *vertShaderTextGLSL =
            "#version 400\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding = 0) uniform vertexBuffer {\n"
            "    mat4 ProjectionMatrix;\n"
            "} myVertexBuffer;\n"
            "layout (location = 0) in vec4 pos;\n"
            "layout (location = 1) in vec2 inTexCoord;\n"
            "layout (location = 2) in vec4 inColor;\n"
            "layout (location = 0) out vec2 outTexCoord;\n"
            "layout (location = 1) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = inColor;\n"
            "   outTexCoord = inTexCoord;\n"
            "   gl_Position = myVertexBuffer.ProjectionMatrix * pos;\n"
            "}\n";


        // Pixel shader
        const char* fragShaderTextGLSL =
            "#version 400\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec2 inTexCoord;\n"
            "layout (location = 1) in vec4 inColor;\n"
            "\n"
            "layout (location = 0) out vec4 outColor;\n"
            "\n"
            "layout(set=0, binding=1) uniform texture2D sTexture;\n"
            "layout(set=0, binding=2) uniform sampler sSampler;\n"
            "\n"
            "void main() {\n"
            "#if 1\n"
            "   outColor = inColor * texture(sampler2D(sTexture, sSampler), inTexCoord.st);\n"
            "   const float gamma = 2.2f;\n"
            "   outColor.xyz = pow(outColor.xyz, vec3(gamma, gamma, gamma));\n"
            "#else\n"
            "   outColor = inColor;\n"
            "#endif\n"
            "}\n";

        const char* fragShaderTextHLSL =
            "\n"
            "[[vk::binding(1, 0)]] Texture2D texture0;\n"
            "[[vk::binding(2, 0)]] SamplerState sampler0;\n"
            "\n"
            "[[vk::location(0)]] float4 main(\n"
            "         [[vk::location(0)]] in float2 uv : TEXCOORD, \n"
            "         [[vk::location(1)]] in float4 col: COLOR)\n"
            ": SV_Target\n"
            "{\n"
            "#if 1\n"
            "   float4 color = col * texture0.Sample(sampler0, uv.xy); \n"
            "   const float gamma = 2.2f;\n"
            "   color.xyz = pow(color.xyz, float3(gamma, gamma, gamma);\n"
            "   return color;\n"
            "#else\n"
            "   return col;\n"
            "#endif\n"
            "}";

        // Compile and create shaders
        DefineList defines;
        VkPipelineShaderStageCreateInfo mVS, mFS;
        VK_CHECK_RESULT(VKCompileFromString(
            pDevice->GetDevice(),
            SST_GLSL,
            VK_SHADER_STAGE_VERTEX_BIT,
            vertShaderTextGLSL,
            "main", "",
            &defines, &mVS));

#define USE_GLSL 1
#ifdef USE_GLSL
        VK_CHECK_RESULT(VKCompileFromString(
            pDevice->GetDevice(),
            SST_GLSL,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            fragShaderTextGLSL,
            "main", "",
            &defines, &mFS));
        
#else
        VK_CHECK_RESULT(VKCompileFromString(pDevice->GetDevice(), SST_HLSL, VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderTextHLSL, "main", "", &defines, &mFS);
#endif
        mShaderStages.clear();
        mShaderStages.push_back(mVS);
        mShaderStages.push_back(mFS);

        // Create descriptor sets
        VkDescriptorSetLayoutBinding descSetLayoutBinding[3];
        descSetLayoutBinding[0].binding = 0;
        descSetLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descSetLayoutBinding[0].descriptorCount = 1;
        descSetLayoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        descSetLayoutBinding[0].pImmutableSamplers = nullptr;

        descSetLayoutBinding[1].binding = 1;
        descSetLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descSetLayoutBinding[1].descriptorCount = 1;
        descSetLayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        descSetLayoutBinding[1].pImmutableSamplers = nullptr;

        descSetLayoutBinding[2].binding = 2;
        descSetLayoutBinding[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descSetLayoutBinding[2].descriptorCount = 1;
        descSetLayoutBinding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        descSetLayoutBinding[2].pImmutableSamplers = nullptr;

        // Next take layout bindings and use them to create a descriptor set layout
        VkDescriptorSetLayoutCreateInfo descSetLayoutCI{};
        descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descSetLayoutCI.pNext = nullptr;
        descSetLayoutCI.flags = 0;
        descSetLayoutCI.bindingCount = 3;
        descSetLayoutCI.pBindings = descSetLayoutBinding;
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(pDevice->GetDevice(), &descSetLayoutCI, nullptr, &mDescLayout));

        // Now use the descriptor layout to create a pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutCI{};
        pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCI.pNext = nullptr;
        pipelineLayoutCI.pushConstantRangeCount = 0;
        pipelineLayoutCI.pPushConstantRanges = nullptr;
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts = &mDescLayout;
        VK_CHECK_RESULT(vkCreatePipelineLayout(pDevice->GetDevice(), &pipelineLayoutCI, nullptr, &mPipelineLayout));
        
        SetResourceName(pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)mPipelineLayout, "ImGUI PL");

        // Create descriptor pool, allocate and update the descriptors
        std::vector<VkDescriptorPoolSize> typeCount = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 128 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 128 },
            { VK_DESCRIPTOR_TYPE_SAMPLER, 128 } };

        VkDescriptorPoolCreateInfo descPoolCI{};
        descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descPoolCI.pNext = nullptr;
        descPoolCI.flags = 0;
        descPoolCI.maxSets = 128 * 3;
        descPoolCI.poolSizeCount = (uint32_t)typeCount.size();
        descPoolCI.pPoolSizes = typeCount.data();
        VK_CHECK_RESULT(vkCreateDescriptorPool(pDevice->GetDevice(), &descPoolCI, nullptr, &mDescriptorPool));

        VkDescriptorSetAllocateInfo descSetAI[1];
        descSetAI[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descSetAI[0].pNext = nullptr;
        descSetAI[0].descriptorPool = mDescriptorPool;
        descSetAI[0].descriptorSetCount = 1;
        descSetAI[0].pSetLayouts = &mDescLayout;

        for (int i = 0; i < 128; i++)
        {
            VK_CHECK_RESULT(vkAllocateDescriptorSets(pDevice->GetDevice(), descSetAI, &mDescriptorSet[i]));

            // update descriptor set, uniforms part
            m_pConstBuf->SetDescriptorSet(0, sizeof(VERTEX_CONSTANT_BUFFER), mDescriptorSet[i]);

            // update descriptor set, image and sample part
            VkDescriptorImageInfo descImage[1] = {};
            descImage[0].sampler = mSampler;
            descImage[0].imageView = mTextureSRV;
            descImage[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet writes[2]{};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].pNext = nullptr;
            writes[0].dstSet = mDescriptorSet[i];
            writes[0].descriptorCount = 1;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            writes[0].pImageInfo = descImage;
            writes[0].dstArrayElement = 0;
            writes[0].dstBinding = 1;

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].pNext = nullptr;
            writes[1].dstSet = mDescriptorSet[i];
            writes[1].descriptorCount = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            writes[1].pImageInfo = descImage;
            writes[1].dstArrayElement = 0;
            writes[1].dstBinding = 2;

            vkUpdateDescriptorSets(m_pDevice->GetDevice(), 2, writes, 0, nullptr);
        }
        UpdatePipeline(renderPass);
    }

    void ImGUI::OnDestroy()
    {
        if (!m_pDevice)
            return;

        vkDestroyImageView(m_pDevice->GetDevice(), mTextureSRV, nullptr);

        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), mDescLayout, nullptr);
        mDescLayout = VK_NULL_HANDLE;

        vkDestroyPipeline(m_pDevice->GetDevice(), mPipeline, nullptr);
        mPipeline = VK_NULL_HANDLE;

        vkDestroyDescriptorPool(m_pDevice->GetDevice(), mDescriptorPool, nullptr);
        mDescriptorPool = VK_NULL_HANDLE;

        if (mTexture2D != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_pDevice->GetAllocator(), mTexture2D, mImageAlloc);
            mTexture2D = VK_NULL_HANDLE;
        }

        vkDestroyPipelineLayout(m_pDevice->GetDevice(), mPipelineLayout, nullptr);
        mPipelineLayout = VK_NULL_HANDLE;

        vkDestroySampler(m_pDevice->GetDevice(), mSampler, nullptr);
        mSampler = VK_NULL_HANDLE;
    }

    void ImGUI::UpdatePipeline(VkRenderPass renderPass)
    {
        if (renderPass == VK_NULL_HANDLE) return;

        if (mPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_pDevice->GetDevice(), mPipeline, nullptr);
            mPipeline = VK_NULL_HANDLE;
        }

        // vertex input state
        VkVertexInputBindingDescription vertexInputBindingDesc{};
        vertexInputBindingDesc.binding = 0;
        vertexInputBindingDesc.stride = sizeof(ImDrawVert);
        vertexInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vi_attrs[3] =
        {
            { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
            { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 2 },
            { 2, 0, VK_FORMAT_R8G8B8A8_UNORM, sizeof(float) * 4 }
        };

        VkPipelineVertexInputStateCreateInfo viStateCI{};
        viStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        viStateCI.pNext = nullptr;
        viStateCI.flags = 0;
        viStateCI.vertexBindingDescriptionCount = 1;
        viStateCI.pVertexBindingDescriptions = &vertexInputBindingDesc;
        viStateCI.vertexAttributeDescriptionCount = _countof(vi_attrs);
        viStateCI.pVertexAttributeDescriptions = vi_attrs;

        // input assembly state
        VkPipelineInputAssemblyStateCreateInfo iaStateCI{};
        iaStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        iaStateCI.pNext = nullptr;
        iaStateCI.flags = 0;
        iaStateCI.primitiveRestartEnable = VK_FALSE;
        iaStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // rasterizer state
        VkPipelineRasterizationStateCreateInfo rasterStateCI{};
        rasterStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterStateCI.pNext = nullptr;
        rasterStateCI.flags = 0;
        rasterStateCI.polygonMode = VK_POLYGON_MODE_FILL;
        rasterStateCI.cullMode = VK_CULL_MODE_NONE;
        rasterStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterStateCI.depthClampEnable = VK_FALSE;
        rasterStateCI.rasterizerDiscardEnable = VK_FALSE;
        rasterStateCI.depthBiasEnable = VK_FALSE;
        rasterStateCI.depthBiasConstantFactor = 0;
        rasterStateCI.depthBiasClamp = 0;
        rasterStateCI.depthBiasSlopeFactor = 0;
        rasterStateCI.lineWidth = 1.0f;

        VkPipelineColorBlendAttachmentState attState[1]{};
        attState[0].blendEnable = VK_TRUE;
        attState[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attState[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attState[0].colorBlendOp = VK_BLEND_OP_ADD;
        attState[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attState[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        attState[0].alphaBlendOp = VK_BLEND_OP_ADD;
        attState[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        // Color blend state
        VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
        colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCI.flags = 0;
        colorBlendStateCI.pNext = nullptr;
        colorBlendStateCI.attachmentCount = 1;
        colorBlendStateCI.pAttachments = attState;
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
        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.pNext = nullptr;
        dynamicState.pDynamicStates = dynamicStateEnables.data();
        dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();

        // view port state
        VkPipelineViewportStateCreateInfo viewportStateCI{};
        viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCI.pNext = nullptr;
        viewportStateCI.flags = 0;
        viewportStateCI.viewportCount = 1;
        viewportStateCI.scissorCount = 1;
        viewportStateCI.pScissors = nullptr;
        viewportStateCI.pViewports = nullptr;

        // depth stencil state

        VkPipelineDepthStencilStateCreateInfo dsStateCI{};
        dsStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        dsStateCI.pNext = nullptr;
        dsStateCI.flags = 0;
        dsStateCI.depthTestEnable = VK_FALSE;
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
        msStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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
        pipelineCI.pRasterizationState = &rasterStateCI;
        pipelineCI.pColorBlendState = &colorBlendStateCI;
        pipelineCI.pTessellationState = nullptr;
        pipelineCI.pMultisampleState = &msStateCI;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.pViewportState = &viewportStateCI;
        pipelineCI.pDepthStencilState = &dsStateCI;
        pipelineCI.pStages = mShaderStages.data();
        pipelineCI.stageCount = (uint32_t)mShaderStages.size();
        pipelineCI.renderPass = renderPass;
        pipelineCI.subpass = 0;
        VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_pDevice->GetDevice(), m_pDevice->GetPipelineCache(), 1, &pipelineCI, nullptr, &mPipeline));
        
        SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)mPipeline, "ImGUI P");
    }

    void ImGUI::Draw(VkCommandBuffer cmdBuffer)
    {
        SetPerfMarkerBegin(cmdBuffer, "ImGUI");

        ImGui::Render();

        if (mPipeline == VK_NULL_HANDLE) return;

        ImDrawData* drawData = ImGui::GetDrawData();

        // Create and grow vertex/index buffers if needed
        char *pVertices = nullptr;
        VkDescriptorBufferInfo VerticesView;
        m_pConstBuf->AllocateVertexBuffer(drawData->TotalVtxCount, sizeof(ImDrawVert), (void **)&pVertices, &VerticesView);

        char *pIndices = nullptr;
        VkDescriptorBufferInfo IndicesView;
        m_pConstBuf->AllocateIndexBuffer(drawData->TotalIdxCount, sizeof(ImDrawIdx), (void **)&pIndices, &IndicesView);

        auto vtxDst = (ImDrawVert*)pVertices;
        auto idxDst = (ImDrawIdx*)pIndices;
        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = drawData->CmdLists[n];
            memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmd_list->VtxBuffer.Size;
            idxDst += cmd_list->IdxBuffer.Size;
        }

        // Setup orthographic projection matrix into our constant buffer
        VkDescriptorBufferInfo ConstantBufferGpuDescriptor;
        {
            VERTEX_CONSTANT_BUFFER* constantBuffer;
            m_pConstBuf->AllocateConstantBuffer(sizeof(VERTEX_CONSTANT_BUFFER), (void **)&constantBuffer, &ConstantBufferGpuDescriptor);

            float L = 0.0f;
            float R = ImGui::GetIO().DisplaySize.x;
            float B = ImGui::GetIO().DisplaySize.y;
            float T = 0.0f;
            float mvp[4][4] =
            {
                { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
                { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
                { 0.0f,         0.0f,           0.5f,       0.0f },
                { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
            };
            memcpy(constantBuffer->mvp, mvp, sizeof(mvp));
        }

        // Setup viewport
        VkViewport viewport;
        viewport.x = 0;
        viewport.y = ImGui::GetIO().DisplaySize.y;
        viewport.width = ImGui::GetIO().DisplaySize.x;
        viewport.height = -(float)(ImGui::GetIO().DisplaySize.y);
        viewport.minDepth = (float)0.0f;
        viewport.maxDepth = (float)1.0f;
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &VerticesView.buffer, &VerticesView.offset);
        vkCmdBindIndexBuffer(cmdBuffer, IndicesView.buffer, IndicesView.offset, VK_INDEX_TYPE_UINT16);

        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

        uint32_t uniformOffsets[1] = { (uint32_t)ConstantBufferGpuDescriptor.offset };
        ImTextureID texID = nullptr;

        // Render command lists
        int vtx_offset = 0;
        int idx_offset = 0;
        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmdList, pcmd);
                }
                else
                {
                    VkRect2D scissor;
                    scissor.offset.x = (int32_t)pcmd->ClipRect.x;
                    scissor.offset.y = (int32_t)pcmd->ClipRect.y;
                    scissor.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                    scissor.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

                    // get a new descriptor (from the 128 we allocated) and update it with the texture we want to use for rendering.
                    if (texID != pcmd->TextureId)
                    {
                        texID = pcmd->TextureId;

                        VkDescriptorImageInfo descImage[1] = {};
                        descImage[0].sampler = mSampler;
                        descImage[0].imageView = (VkImageView)pcmd->TextureId;
                        descImage[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        VkWriteDescriptorSet writes[2];
                        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writes[0].pNext = nullptr;
                        writes[0].dstSet = mDescriptorSet[mCurrentDescriptorIndex];
                        writes[0].descriptorCount = 1;
                        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        writes[0].pImageInfo = descImage;
                        writes[0].dstArrayElement = 0;
                        writes[0].dstBinding = 1;

                        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writes[1].pNext = nullptr;
                        writes[1].dstSet = mDescriptorSet[mCurrentDescriptorIndex];
                        writes[1].descriptorCount = 1;
                        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                        writes[1].pImageInfo = descImage;
                        writes[1].dstArrayElement = 0;
                        writes[1].dstBinding = 2;

                        vkUpdateDescriptorSets(m_pDevice->GetDevice(), 2, writes, 0, nullptr);

                        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSet[mCurrentDescriptorIndex], 1, uniformOffsets);

                        mCurrentDescriptorIndex++;
                        mCurrentDescriptorIndex &= 127;
                    }

                    vkCmdDrawIndexed(cmdBuffer, pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
                }
                idx_offset += (int)pcmd->ElemCount;
            }
            vtx_offset += cmdList->VtxBuffer.Size;
        }
        SetPerfMarkerEnd(cmdBuffer);
    }
}

