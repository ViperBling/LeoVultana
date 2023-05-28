#pragma once

#include "PCHVK.h"

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << LeoVultana_VK::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

namespace LeoVultana_VK
{
    std::string errorString(VkResult res);

    // this is when you need to clear the attachment, for example when you are not rendering the full screen.
    void AttachClearBeforeUse(
        VkFormat format, VkSampleCountFlagBits sampleCount,
        VkImageLayout initialLayout, VkImageLayout finalLayout,
        VkAttachmentDescription *pAttachDesc);
    // No clear, attachment will keep data that was not written (if this is the first pass make sure you are filling the whole screen)
    void AttachNoClearBeforeUse(
        VkFormat format, VkSampleCountFlagBits sampleCount,
        VkImageLayout initialLayout, VkImageLayout finalLayout,
        VkAttachmentDescription *pAttachDesc);
    // Attanchment where we will be using alpha blending, this means we care about the previous contents
    void AttachBlending(
        VkFormat format, VkSampleCountFlagBits sampleCount,
        VkImageLayout initialLayout, VkImageLayout finalLayout,
        VkAttachmentDescription *pAttachDesc);
    VkRenderPass CreateRenderPassOptimal(
        VkDevice device, uint32_t colorAttachments,
        VkAttachmentDescription *pColorAttachments,
        VkAttachmentDescription *pDepthAttachment);

    // Sets the viewport and scissors to a fixed height and width
    void SetViewportAndScissor(VkCommandBuffer cmdBuffer, uint32_t topX, uint32_t topY, uint32_t width, uint32_t height);

    // Creates a Render pass that will discard the contents of the render target.
    VkRenderPass SimpleColorWriteRenderPass(
        VkDevice device, VkImageLayout initialLayout,
        VkImageLayout passLayout, VkImageLayout finalLayout);

    // Creates a Render pass that will use the contents of the render target for blending.
    VkRenderPass SimpleColorBlendRenderPass(
        VkDevice device, VkImageLayout initialLayout,
        VkImageLayout passLayout, VkImageLayout finalLayout);

    // Sets the i-th Descriptor Set entry to use a given image view + sampler. The sampler can be null is a static one is being used.
    void SetDescriptorSet(
        VkDevice device,
        uint32_t index,
        VkImageView imageView,
        VkSampler *pSampler,
        VkDescriptorSet descriptorSet);
    void SetDescriptorSet(
        VkDevice device,
        uint32_t index,
        uint32_t descriptorsCount,
        const std::vector<VkImageView>& imageViews,
        VkSampler* pSampler,
        VkDescriptorSet descriptorSet);
    void SetDescriptorSet(
        VkDevice device,
        uint32_t index,
        VkImageView imageView,
        VkDescriptorSet descriptorSet);
    void SetDescriptorSetForDepth(
        VkDevice device,
        uint32_t index,
        VkImageView imageView,
        VkSampler *pSampler,
        VkDescriptorSet descriptorSet);

    VkFramebuffer CreateFrameBuffer(
        VkDevice device,
        VkRenderPass renderPass,
        const std::vector<VkImageView> *pAttachments,
        uint32_t Width, uint32_t Height);
}