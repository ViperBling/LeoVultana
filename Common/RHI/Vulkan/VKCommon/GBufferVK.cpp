#include "PCHVK.h"
#include "GBufferVK.h"
#include "HelperVK.h"

using namespace LeoVultana_VK;

// ========================================== GBufferRenderPass ========================================== //
void GBufferRenderPass::OnCreate(GBuffer *pGBuffer, GBufferFlags flags, bool bClear, const std::string &name)
{
    mFlags = flags;
    m_pGBuffer = pGBuffer;
    m_pDevice = pGBuffer->GetDevice();
    mRenderPass = pGBuffer->CreateRenderPass(flags, bClear);

    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)mRenderPass, name.c_str());
}

void GBufferRenderPass::OnDestroy()
{
    vkDestroyRenderPass(m_pGBuffer->GetDevice()->GetDevice(), mRenderPass, nullptr);
}

void GBufferRenderPass::OnCreateWindowSizeDependentResources(uint32_t width, uint32_t height)
{
    std::vector<VkImageView> attachments;
    m_pGBuffer->GetAttachmentList(mFlags, &attachments, &mClearValues);
    mFrameBuffer = CreateFrameBuffer(m_pGBuffer->GetDevice()->GetDevice(), mRenderPass, &attachments, width, height);
}

void GBufferRenderPass::OnDestroyWindowSizeDependentResources()
{
    vkDestroyFramebuffer(m_pGBuffer->GetDevice()->GetDevice(), mFrameBuffer, nullptr);
}

void GBufferRenderPass::BeginPass(VkCommandBuffer commandList, VkRect2D renderArea)
{
    VkRenderPassBeginInfo renderPassBI{};
    renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.pNext = nullptr;
    renderPassBI.renderPass = mRenderPass;
    renderPassBI.framebuffer = mFrameBuffer;
    renderPassBI.renderArea = renderArea;
    renderPassBI.pClearValues = mClearValues.data();
    renderPassBI.clearValueCount = static_cast<uint32_t>(mClearValues.size());
    vkCmdBeginRenderPass(commandList, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

    SetViewportAndScissor(commandList, renderArea.offset.x, renderArea.offset.y, renderArea.extent.width, renderArea.extent.height);
}

void GBufferRenderPass::EndPass(VkCommandBuffer commandList)
{
    vkCmdEndRenderPass(commandList);
}

void GBufferRenderPass::GetCompilerDefines(DefineList &defines)
{
    int rtIndex = 0;

    // GDR (Forward pass)
    if (mFlags & GBUFFER_FORWARD)
    {
        defines["HAS_FORWARD_RT"] = std::to_string(rtIndex++);
    }

    // Motion Vectors
    if (mFlags & GBUFFER_MOTION_VECTORS)
    {
        defines["HAS_MOTION_VECTORS"] = std::to_string(1);
        defines["HAS_MOTION_VECTORS_RT"] = std::to_string(rtIndex++);
    }

    // Normal Buffer
    if (mFlags & GBUFFER_NORMAL_BUFFER)
    {
        defines["HAS_NORMALS_RT"] = std::to_string(rtIndex++);
    }

    // Diffuse
    if (mFlags & GBUFFER_DIFFUSE)
    {
        defines["HAS_DIFFUSE_RT"] = std::to_string(rtIndex++);
    }

    // Specular roughness
    if (mFlags & GBUFFER_SPECULAR_ROUGHNESS)
    {
        defines["HAS_SPECULAR_ROUGHNESS_RT"] = std::to_string(rtIndex++);
    }
}

VkSampleCountFlagBits GBufferRenderPass::GetSampleCount()
{
    return m_pGBuffer->GetSampleCount();
}

// ========================================== GBuffer ========================================== //
void GBuffer::OnCreate(
    Device *pDevice,
    ResourceViewHeaps *pHeaps,
    const std::map<GBufferFlags, VkFormat> &formats,
    int sampleCount)
{
    mGBufferFlags = GBUFFER_NONE;
    for (auto a : formats) mGBufferFlags = mGBufferFlags | a.first;

    m_pDevice = pDevice;
    mSampleCount = (VkSampleCountFlagBits)sampleCount;
    mFormats = formats;
}

void GBuffer::OnDestroy()
{
}

void GBuffer::OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t width, uint32_t height)
{
    // Create Texture + RTV, to hold the resolved scene
    if (mGBufferFlags & GBUFFER_FORWARD)
    {
        mHDR.InitRenderTarget(
            m_pDevice, width, height,
            mFormats[GBUFFER_FORWARD], mSampleCount,
            (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
            false, "mHDR");
        mHDR.CreateSRV(&mHDRSRV);
    }

    // Motion Vectors
    if (mGBufferFlags & GBUFFER_MOTION_VECTORS)
    {
        mMotionVectors.InitRenderTarget(
            m_pDevice, width, height, mFormats[GBUFFER_MOTION_VECTORS], mSampleCount,
            (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
            false, "mMotionVectors");
        mMotionVectors.CreateSRV(&mMotionVectorsSRV);
    }

    // Normal Buffer
    if (mGBufferFlags & GBUFFER_NORMAL_BUFFER)
    {
        mNormalBuffer.InitRenderTarget(
            m_pDevice, width, height, mFormats[GBUFFER_NORMAL_BUFFER], mSampleCount,
            (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
            false, "mNormalBuffer");
        mNormalBuffer.CreateSRV(&mNormalBufferSRV);
    }

    // Diffuse
    if (mGBufferFlags & GBUFFER_DIFFUSE)
    {
        mDiffuse.InitRenderTarget(
            m_pDevice, width, height, mFormats[GBUFFER_DIFFUSE], mSampleCount,
            (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
            false, "mDiffuse");
        mDiffuse.CreateSRV(&mDiffuseSRV);
    }

    // Specular Roughness
    if (mGBufferFlags & GBUFFER_SPECULAR_ROUGHNESS)
    {
        mSpecularRoughness.InitRenderTarget(
            m_pDevice, width, height, mFormats[GBUFFER_SPECULAR_ROUGHNESS], mSampleCount,
            (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
            false, "mSpecularRoughness");
        mSpecularRoughness.CreateSRV(&mSpecularRoughnessSRV);
    }

    // Create depth buffer
    if (mGBufferFlags & GBUFFER_DEPTH)
    {
        mDepthBuffer.InitDepthStencil(m_pDevice, width, height, mFormats[GBUFFER_DEPTH], mSampleCount, "mDepthBuffer");
        mDepthBuffer.CreateDSV(&mDepthBufferDSV);
        mDepthBuffer.CreateRTV(&mDepthBufferSRV);
    }
}

void GBuffer::OnDestroyWindowSizeDependentResources()
{
    if (mGBufferFlags & GBUFFER_SPECULAR_ROUGHNESS)
    {
        vkDestroyImageView(m_pDevice->GetDevice(), mSpecularRoughnessSRV, nullptr);
        mSpecularRoughness.OnDestroy();
    }

    if (mGBufferFlags & GBUFFER_DIFFUSE)
    {
        vkDestroyImageView(m_pDevice->GetDevice(), mDiffuseSRV, nullptr);
        mDiffuse.OnDestroy();
    }

    if (mGBufferFlags & GBUFFER_NORMAL_BUFFER)
    {
        vkDestroyImageView(m_pDevice->GetDevice(), mNormalBufferSRV, nullptr);
        mNormalBuffer.OnDestroy();
    }

    if (mGBufferFlags & GBUFFER_MOTION_VECTORS)
    {
        vkDestroyImageView(m_pDevice->GetDevice(), mMotionVectorsSRV, nullptr);
        mMotionVectors.OnDestroy();
    }

    if (mGBufferFlags & GBUFFER_FORWARD)
    {
        vkDestroyImageView(m_pDevice->GetDevice(), mHDRSRV, nullptr);
        mHDR.OnDestroy();
    }

    if (mGBufferFlags & GBUFFER_DEPTH)
    {
        vkDestroyImageView(m_pDevice->GetDevice(), mDepthBufferDSV, nullptr);
        vkDestroyImageView(m_pDevice->GetDevice(), mDepthBufferSRV, nullptr);
        mDepthBuffer.OnDestroy();
    }
}

void GBuffer::GetAttachmentList(
    GBufferFlags flags, std::vector<VkImageView> *pAttachments,
    std::vector<VkClearValue> *pClearValues)
{
    pAttachments->clear();

    // Create Texture + RTV, to hold the resolved scene
    if (flags & GBUFFER_FORWARD)
    {
        pAttachments->push_back(mHDRSRV);

        if (pClearValues)
        {
            VkClearValue cv;
            cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
            pClearValues->push_back(cv);
        }
    }

    // Motion Vectors
    if (flags & GBUFFER_MOTION_VECTORS)
    {
        pAttachments->push_back(mMotionVectorsSRV);

        if (pClearValues)
        {
            VkClearValue cv;
            cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
            pClearValues->push_back(cv);
        }
    }

    // Normal Buffer
    if (flags & GBUFFER_NORMAL_BUFFER)
    {
        pAttachments->push_back(mNormalBufferSRV);

        if (pClearValues)
        {
            VkClearValue cv;
            cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
            pClearValues->push_back(cv);
        }
    }

    // Diffuse
    if (flags & GBUFFER_DIFFUSE)
    {
        pAttachments->push_back(mDiffuseSRV);

        if (pClearValues)
        {
            VkClearValue cv;
            cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
            pClearValues->push_back(cv);
        }
    }

    // Specular Roughness
    if (flags & GBUFFER_SPECULAR_ROUGHNESS)
    {
        pAttachments->push_back(mSpecularRoughnessSRV);

        if (pClearValues)
        {
            VkClearValue cv;
            cv.color = { 0.0f, 0.0f, 0.0f, 0.0f };
            pClearValues->push_back(cv);
        }
    }

    // Create depth buffer
    if (flags & GBUFFER_DEPTH)
    {
        pAttachments->push_back(mDepthBufferDSV);

        if (pClearValues)
        {
            VkClearValue cv;
            cv.depthStencil = { 1.0f, 0 };
            pClearValues->push_back(cv);
        }
    }
}

VkRenderPass GBuffer::CreateRenderPass(GBufferFlags flags, bool bClear)
{
    VkAttachmentDescription depthAttachment;
    VkAttachmentDescription colorAttachments[10];
    uint32_t colorAttachmentCount = 0;

    auto addAttachment = bClear ? AttachClearBeforeUse : AttachBlending;
    VkImageLayout previousColor = bClear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkImageLayout previousDepth = bClear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if (flags & GBUFFER_FORWARD)
    {
        addAttachment(mFormats[GBUFFER_FORWARD], mSampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttachmentCount++]);
        assert(mGBufferFlags & GBUFFER_FORWARD); // asserts if there is the RT is not present in the GBuffer
    }

    if (flags & GBUFFER_MOTION_VECTORS)
    {
        addAttachment(mFormats[GBUFFER_MOTION_VECTORS], mSampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttachmentCount++]);
        assert(mGBufferFlags & GBUFFER_MOTION_VECTORS); // asserts if there is the RT is not present in the GBuffer
    }

    if (flags & GBUFFER_NORMAL_BUFFER)
    {
        addAttachment(mFormats[GBUFFER_NORMAL_BUFFER], mSampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttachmentCount++]);
        assert(mGBufferFlags & GBUFFER_NORMAL_BUFFER); // asserts if there is the RT is not present in the GBuffer
    }

    if (flags & GBUFFER_DIFFUSE)
    {
        addAttachment(mFormats[GBUFFER_DIFFUSE], mSampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttachmentCount++]);
        assert(mGBufferFlags & GBUFFER_DIFFUSE); // asserts if there is the RT is not present in the GBuffer
    }

    if (flags & GBUFFER_SPECULAR_ROUGHNESS)
    {
        addAttachment(mFormats[GBUFFER_SPECULAR_ROUGHNESS], mSampleCount, previousColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &colorAttachments[colorAttachmentCount++]);
        assert(mGBufferFlags & GBUFFER_SPECULAR_ROUGHNESS); // asserts if there is the RT is not present in the GBuffer
    }

    if (flags & GBUFFER_DEPTH)
    {
        addAttachment(mFormats[GBUFFER_DEPTH], mSampleCount, previousDepth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, &depthAttachment);
        assert(mGBufferFlags & GBUFFER_DEPTH); // asserts if there is the RT is not present in the GBuffer
    }

    return CreateRenderPassOptimal(m_pDevice->GetDevice(), colorAttachmentCount, colorAttachments, &depthAttachment);
}