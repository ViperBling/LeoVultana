#include "PCHVK.h"
#include "HelperVK.h"
#include "ExtDebugUtilsVK.h"

namespace LeoVultana_VK
{
    std::string errorString(VkResult res)
    {
        switch (res)
        {
#define STR(r) case VK_ ##r: return #r
            STR(NOT_READY);
            STR(TIMEOUT);
            STR(EVENT_SET);
            STR(EVENT_RESET);
            STR(INCOMPLETE);
            STR(ERROR_OUT_OF_HOST_MEMORY);
            STR(ERROR_OUT_OF_DEVICE_MEMORY);
            STR(ERROR_INITIALIZATION_FAILED);
            STR(ERROR_DEVICE_LOST);
            STR(ERROR_MEMORY_MAP_FAILED);
            STR(ERROR_LAYER_NOT_PRESENT);
            STR(ERROR_EXTENSION_NOT_PRESENT);
            STR(ERROR_FEATURE_NOT_PRESENT);
            STR(ERROR_INCOMPATIBLE_DRIVER);
            STR(ERROR_TOO_MANY_OBJECTS);
            STR(ERROR_FORMAT_NOT_SUPPORTED);
            STR(ERROR_SURFACE_LOST_KHR);
            STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
            STR(SUBOPTIMAL_KHR);
            STR(ERROR_OUT_OF_DATE_KHR);
            STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
            STR(ERROR_VALIDATION_FAILED_EXT);
            STR(ERROR_INVALID_SHADER_NV);
#undef STR
            default:
                return "UNKNOWN_ERROR";
        }
    }

    void AttachClearBeforeUse(
        VkFormat format,
        VkSampleCountFlagBits sampleCount,
        VkImageLayout initialLayout,
        VkImageLayout finalLayout,
        VkAttachmentDescription *pAttachDesc)
    {
        pAttachDesc->format = format;
        pAttachDesc->samples = sampleCount;
        pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               // 清除操作
        pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pAttachDesc->initialLayout = initialLayout;
        pAttachDesc->finalLayout = finalLayout;
        pAttachDesc->flags = 0;
    }

    void AttachNoClearBeforeUse(
        VkFormat format,
        VkSampleCountFlagBits sampleCount,
        VkImageLayout initialLayout,
        VkImageLayout finalLayout,
        VkAttachmentDescription *pAttachDesc)
    {
        pAttachDesc->format = format;
        pAttachDesc->samples = sampleCount;
        pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;          // 不清除
        pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pAttachDesc->initialLayout = initialLayout;
        pAttachDesc->finalLayout = finalLayout;
        pAttachDesc->flags = 0;
    }

    void AttachBlending(
        VkFormat format,
        VkSampleCountFlagBits sampleCount,
        VkImageLayout initialLayout,
        VkImageLayout finalLayout,
        VkAttachmentDescription *pAttachDesc)
    {
        pAttachDesc->format = format;
        pAttachDesc->samples = sampleCount;
        pAttachDesc->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        pAttachDesc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pAttachDesc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pAttachDesc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pAttachDesc->initialLayout = initialLayout;
        pAttachDesc->finalLayout = finalLayout;
        pAttachDesc->flags = 0;
    }

    VkRenderPass CreateRenderPassOptimal(
        VkDevice device,
        uint32_t colorAttachments,
        VkAttachmentDescription *pColorAttachments,
        VkAttachmentDescription *pDepthAttachment)
    {
        // we need to put all the color and the depth attachments in the same buffer
        VkAttachmentDescription  attachmentDescs[10];
        assert(colorAttachments < 10);

        memcpy(attachmentDescs, pColorAttachments, sizeof(VkAttachmentDescription) * colorAttachments);
        if (pDepthAttachment)
            memcpy(&attachmentDescs[colorAttachments], pDepthAttachment, sizeof(VkAttachmentDescription));

        // Create reference for attachments
        VkAttachmentReference colorReferences[10];
        for (uint32_t i = 0; i < colorAttachments; i++)
            colorReferences[i] = { i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkAttachmentReference depthReference = { colorAttachments, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
        // Create Subpass
        VkSubpassDescription subpassDesc{};
        subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.flags = 0;
        subpassDesc.inputAttachmentCount = 0;
        subpassDesc.pInputAttachments = nullptr;
        subpassDesc.colorAttachmentCount = colorAttachments;
        subpassDesc.pColorAttachments = colorReferences;
        subpassDesc.pResolveAttachments = nullptr;
        subpassDesc.pDepthStencilAttachment = (pDepthAttachment)? &depthReference : nullptr;
        subpassDesc.preserveAttachmentCount = 0;
        subpassDesc.pPreserveAttachments = nullptr;

        VkSubpassDependency subpassDependency{};
        subpassDependency.dependencyFlags = 0;
        subpassDependency.dstAccessMask =
            VK_ACCESS_SHADER_READ_BIT |
            ((colorAttachments) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
            ((pDepthAttachment) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
        subpassDependency.dstStageMask =
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            ((colorAttachments) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0) |
            ((pDepthAttachment) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 0);
        subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.srcAccessMask =
            ((colorAttachments) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0) |
            ((pDepthAttachment) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
        subpassDependency.srcStageMask =
            ((colorAttachments) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0) |
            ((pDepthAttachment) ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 0);
        subpassDependency.srcSubpass = 0;

        // Create Render Pass
        VkRenderPassCreateInfo renderPassCI{};
        renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCI.pNext = nullptr;
        renderPassCI.attachmentCount = colorAttachments;
        if (pDepthAttachment != nullptr)
            renderPassCI.attachmentCount++;
        renderPassCI.pAttachments = attachmentDescs;
        renderPassCI.subpassCount = 1;
        renderPassCI.pSubpasses = &subpassDesc;
        renderPassCI.dependencyCount = 1;
        renderPassCI.pDependencies = &subpassDependency;

        VkRenderPass renderPass;
        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));

        SetResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass, "CreateRenderPassOptimal");

        return renderPass;
    }

    void SetViewportAndScissor(
        VkCommandBuffer cmdBuffer,
        uint32_t topX, uint32_t topY,
        uint32_t width, uint32_t height)
    {
        VkViewport viewport;
        viewport.x = static_cast<float>(topX);
        viewport.y = static_cast<float>(topY) + static_cast<float>(height);
        viewport.width = static_cast<float>(width);
        viewport.height = -static_cast<float>(height);
        viewport.minDepth = (float)0.0f;
        viewport.maxDepth = (float)1.0f;
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor;
        scissor.extent.width = (uint32_t)(width);
        scissor.extent.height = (uint32_t)(height);
        scissor.offset.x = (int32_t)topX;
        scissor.offset.y = (int32_t)topY;
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
    }

    VkRenderPass SimpleColorWriteRenderPass(
        VkDevice device,
        VkImageLayout initialLayout,
        VkImageLayout passLayout,
        VkImageLayout finalLayout)
    {
        // color RT
        VkAttachmentDescription attachmentDescs[1];
        attachmentDescs[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
        attachmentDescs[0].samples = VK_SAMPLE_COUNT_1_BIT;
        // we don't care about the previous contents, this is for a full screen pass with no blending
        attachmentDescs[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescs[0].initialLayout = initialLayout;
        attachmentDescs[0].finalLayout = finalLayout;
        attachmentDescs[0].flags = 0;

        VkAttachmentReference colorReference = { 0, passLayout };

        VkSubpassDescription subpassDesc{};
        subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.flags = 0;
        subpassDesc.inputAttachmentCount = 0;
        subpassDesc.pInputAttachments = nullptr;
        subpassDesc.colorAttachmentCount = 1;
        subpassDesc.pColorAttachments = &colorReference;
        subpassDesc.pResolveAttachments = nullptr;
        subpassDesc.pDepthStencilAttachment = nullptr;
        subpassDesc.preserveAttachmentCount = 0;
        subpassDesc.pPreserveAttachments = nullptr;

        VkSubpassDependency subpassDep{};
        subpassDep.dependencyFlags = 0;
        subpassDep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDep.srcSubpass = 0;
        subpassDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        subpassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        subpassDep.dstSubpass = VK_SUBPASS_EXTERNAL;

        VkRenderPassCreateInfo renderPassCI = {};
        renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCI.pNext = nullptr;
        renderPassCI.attachmentCount = 1;
        renderPassCI.pAttachments = attachmentDescs;
        renderPassCI.subpassCount = 1;
        renderPassCI.pSubpasses = &subpassDesc;
        renderPassCI.dependencyCount = 1;
        renderPassCI.pDependencies = &subpassDep;

        VkRenderPass renderPass;
        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));

        SetResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass, "SimpleColorWriteRenderPass");

        return renderPass;
    }

    VkRenderPass SimpleColorBlendRenderPass(
        VkDevice device,
        VkImageLayout initialLayout,
        VkImageLayout passLayout,
        VkImageLayout finalLayout)
    {
        // color RT
        VkAttachmentDescription attachmentDescs[1];
        attachmentDescs[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
        attachmentDescs[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescs[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachmentDescs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescs[0].initialLayout = initialLayout;
        attachmentDescs[0].finalLayout = finalLayout;
        attachmentDescs[0].flags = 0;

        VkAttachmentReference colorReference = { 0, passLayout };

        VkSubpassDescription subpassDesc{};
        subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.flags = 0;
        subpassDesc.inputAttachmentCount = 0;
        subpassDesc.pInputAttachments = nullptr;
        subpassDesc.colorAttachmentCount = 1;
        subpassDesc.pColorAttachments = &colorReference;
        subpassDesc.pResolveAttachments = nullptr;
        subpassDesc.pDepthStencilAttachment = nullptr;
        subpassDesc.preserveAttachmentCount = 0;
        subpassDesc.pPreserveAttachments = nullptr;

        VkSubpassDependency subpassDep{};
        subpassDep.dependencyFlags = 0;
        subpassDep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDep.srcSubpass = 0;
        subpassDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        subpassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        subpassDep.dstSubpass = VK_SUBPASS_EXTERNAL;

        VkRenderPassCreateInfo renderPassCI = {};
        renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCI.pNext = nullptr;
        renderPassCI.attachmentCount = 1;
        renderPassCI.pAttachments = attachmentDescs;
        renderPassCI.subpassCount = 1;
        renderPassCI.pSubpasses = &subpassDesc;
        renderPassCI.dependencyCount = 1;
        renderPassCI.pDependencies = &subpassDep;

        VkRenderPass renderPass;
        VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));

        SetResourceName(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass, "SimpleColorWriteRenderPass");

        return renderPass;
    }

    void SetDescriptorSet(
        VkDevice device,
        uint32_t index,
        VkImageView imageView,
        VkImageLayout imageLayout,
        VkSampler* pSampler,
        VkDescriptorSet descriptorSet)
    {
        VkDescriptorImageInfo descImageInfo{};
        descImageInfo.sampler = (pSampler == nullptr) ? VK_NULL_HANDLE : *pSampler;
        descImageInfo.imageView = imageView;
        descImageInfo.imageLayout = imageLayout;

        VkWriteDescriptorSet writeDescSet{};
        writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescSet.pNext = nullptr;
        writeDescSet.dstSet = descriptorSet;
        writeDescSet.descriptorCount = 1;
        writeDescSet.descriptorType = (pSampler == nullptr) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescSet.pImageInfo = &descImageInfo;
        writeDescSet.dstBinding = index;
        writeDescSet.dstArrayElement = 0;

        vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
    }

    void SetDescriptorSet(
        VkDevice device,
        uint32_t index,
        uint32_t descriptorsCount,
        const std::vector<VkImageView>& imageViews,
        VkImageLayout imageLayout,
        VkSampler* pSampler,
        VkDescriptorSet descriptorSet)
    {
        std::vector<VkDescriptorImageInfo> descImageInfos(descriptorsCount);
        uint32_t i = 0;
        for (; i < imageViews.size(); ++i)
        {
            descImageInfos[i].sampler = (pSampler == nullptr) ? VK_NULL_HANDLE : *pSampler;
            descImageInfos[i].imageView = imageViews[i];
            descImageInfos[i].imageLayout = imageLayout;
        }
        // we should still assign the remaining descriptors
        // Using the VK_EXT_robustness2 extension, it is possible to assign a NULL one
        for (; i < descriptorsCount; ++i)
        {
            descImageInfos[i].sampler = (pSampler == nullptr) ? VK_NULL_HANDLE : *pSampler;
            descImageInfos[i].imageView = VK_NULL_HANDLE;
            descImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        VkWriteDescriptorSet writeDescSet{};
        writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescSet.pNext = nullptr;
        writeDescSet.dstSet = descriptorSet;
        writeDescSet.descriptorCount = descriptorsCount;
        writeDescSet.descriptorType = (pSampler == nullptr) ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescSet.pImageInfo = descImageInfos.data();
        writeDescSet.dstBinding = index;
        writeDescSet.dstArrayElement = 0;

        vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
    }

    void SetDescriptorSet(
        VkDevice device,
        uint32_t index,
        VkImageView imageView,
        VkSampler *pSampler,
        VkDescriptorSet descriptorSet)
    {
        SetDescriptorSet(device, index, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
    }

    void SetDescriptorSet(
        VkDevice device,
        uint32_t index,
        uint32_t descriptorsCount,
        const std::vector<VkImageView> &imageViews,
        VkSampler *pSampler,
        VkDescriptorSet descriptorSet)
    {
        SetDescriptorSet(device, index, descriptorsCount, imageViews, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
    }

    void SetDescriptorSet(
        VkDevice device,
        uint32_t index,
        VkImageView imageView,
        VkDescriptorSet descriptorSet)
    {
        VkDescriptorImageInfo descImageInfo{};
        descImageInfo.sampler = VK_NULL_HANDLE;
        descImageInfo.imageView = imageView;
        descImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet writeDescSet{};
        writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescSet.pNext = nullptr;
        writeDescSet.dstSet = descriptorSet;
        writeDescSet.descriptorCount = 1;
        writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescSet.pImageInfo = &descImageInfo;
        writeDescSet.dstBinding = index;
        writeDescSet.dstArrayElement = 0;

        vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
    }

    void SetDescriptorSetForDepth(
        VkDevice device,
        uint32_t index,
        VkImageView imageView,
        VkSampler *pSampler,
        VkDescriptorSet descriptorSet)
    {
        SetDescriptorSet(device, index, imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, pSampler, descriptorSet);
    }

    VkFramebuffer CreateFrameBuffer(
        VkDevice device,
        VkRenderPass renderPass,
        const std::vector<VkImageView> *pAttachments,
        uint32_t Width, uint32_t Height)
    {
        VkFramebufferCreateInfo frameBufferCI = {};
        frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCI.layers = 1;
        frameBufferCI.pNext = nullptr;
        frameBufferCI.width = Width;
        frameBufferCI.height = Height;
        frameBufferCI.renderPass = renderPass;
        frameBufferCI.pAttachments = pAttachments->data();
        frameBufferCI.attachmentCount = (uint32_t)pAttachments->size();

        VkFramebuffer frameBuffer;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCI, nullptr, &frameBuffer));

        SetResourceName(device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)frameBuffer, "HelperCreateFrameBuffer");

        return frameBuffer;
    }

    void BeginRenderPass(
        VkCommandBuffer commandList,
        VkRenderPass renderPass,
        VkFramebuffer frameBuffer,
        const std::vector<VkClearValue> *pClearValues,
        uint32_t Width, uint32_t Height)
    {
        VkRenderPassBeginInfo renderPassBI{};
        renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBI.pNext = nullptr;
        renderPassBI.renderPass = renderPass;
        renderPassBI.framebuffer = frameBuffer;
        renderPassBI.renderArea.offset.x = 0;
        renderPassBI.renderArea.offset.y = 0;
        renderPassBI.renderArea.extent.width = Width;
        renderPassBI.renderArea.extent.height = Height;
        renderPassBI.pClearValues = pClearValues->data();
        renderPassBI.clearValueCount = (uint32_t)pClearValues->size();
        vkCmdBeginRenderPass(commandList, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
    }
}


