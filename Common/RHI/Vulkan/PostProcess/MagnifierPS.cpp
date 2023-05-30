#include "PCHVK.h"
#include "MagnifierPS.h"
#include "DeviceVK.h"
#include "DynamicBufferRingVK.h"
#include "ExtDebugUtilsVK.h"
#include "HelperVK.h"
#include "SwapChainVK.h"
#include "Misc.h"

using namespace LeoVultana_VK;

static const char* SHADER_FILE_NAME_MAGNIFIER = "MagnifierPS.glsl";
static const char* SHADER_ENTRY_POINT;
using cbHandle_t = VkDescriptorBufferInfo;

void MagnifierPS::OnCreate(
    Device *pDevice,
    ResourceViewHeaps *pResourceViewHeaps,
    DynamicBufferRing *pDynamicBufferRing,
    StaticBufferPool *pStaticBufferPool,
    VkFormat outFormat,
    bool bOutputsToSwapChain)
{
    m_pDevice = pDevice;
    m_pResourceViewHeaps = pResourceViewHeaps;
    m_pDynamicBufferRing = pDynamicBufferRing;
    m_bOutToSwapChain = bOutputsToSwapChain;

    InitializeDescriptorSets();

    mRenderPass = SimpleColorBlendRenderPass(
        m_pDevice->GetDevice(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    CompileShaders(pStaticBufferPool, outFormat);
}

void MagnifierPS::OnDestroy()
{
    vkDestroyRenderPass(m_pDevice->GetDevice(), mRenderPass, nullptr);
    DestroyShaders();
    DestroyDescriptorSets();
}

void MagnifierPS::OnCreateWindowSizeDependentResources(Texture *pTexture)
{
    pTexture->CreateSRV(&mImageView);
    UpdateDescriptorSets(mImageView);

    if (!m_bOutToSwapChain)
    {
        VkDevice device = m_pDevice->GetDevice();
        const uint32_t imgWidth = pTexture->GetWidth();
        const uint32_t imgHeight = pTexture->GetHeight();

        // create pass output image and its views
        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.pNext = nullptr;
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.format = pTexture->GetFormat();
        imageCI.extent.width = imgWidth;
        imageCI.extent.height = imgHeight;
        imageCI.extent.depth = 1;
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.queueFamilyIndexCount = 0;
        imageCI.pQueueFamilyIndices = nullptr;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCI.usage = (VkImageUsageFlags)(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        imageCI.flags = 0;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        mTexturePassOut.Init(m_pDevice, &imageCI, "TexMagnifierOutput");
        mTexturePassOut.CreateSRV(&mSRVOut);
        mTexturePassOut.CreateRTV(&mRTVOut);

        // create framebuffer
        VkImageView attachments[] = { mRTVOut };
        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.pNext = nullptr;
        fb_info.renderPass = mRenderPass;
        fb_info.attachmentCount = 1;
        fb_info.pAttachments = attachments;
        fb_info.width = imgWidth;
        fb_info.height = imgHeight;
        fb_info.layers = 1;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &fb_info, nullptr, &mFrameBuffer))
        SetResourceName(device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)mFrameBuffer, "MagnifierPS");
    }
}

void MagnifierPS::OnDestroyWindowSizeDependentResources()
{
    VkDevice device = m_pDevice->GetDevice();

    vkDestroyImageView(m_pDevice->GetDevice(), mImageView, nullptr);
    vkDestroyImageView(device, mSRVOut, nullptr);
    vkDestroySampler(m_pDevice->GetDevice(), mSampler, nullptr);

    if (!m_bOutToSwapChain)
    {
        vkDestroyImageView(device, mRTVOut, nullptr);
        mTexturePassOut.OnDestroy();
        vkDestroyFramebuffer(device, mFrameBuffer, nullptr);
    }
}

void MagnifierPS::BeginPass(VkCommandBuffer cmdBuffer, VkRect2D renderArea, SwapChain *pSwapChain)
{
    VkRenderPassBeginInfo renderPassBI{};
    renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.pNext = nullptr;
    renderPassBI.renderPass = pSwapChain ? pSwapChain->GetRenderPass() : mRenderPass;
    renderPassBI.framebuffer = pSwapChain ? pSwapChain->GetCurrentFrameBuffer() : mFrameBuffer;
    renderPassBI.renderArea = renderArea;
    renderPassBI.clearValueCount = 0;
    renderPassBI.pClearValues = nullptr;

    vkCmdBeginRenderPass(cmdBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);
}

void MagnifierPS::EndPass(VkCommandBuffer cmdBuffer)
{
    vkCmdEndRenderPass(cmdBuffer);
}

void MagnifierPS::Draw(VkCommandBuffer cmdBuffer, const MagnifierPS::PassParameters &params, SwapChain *pSwapChain)
{
    SetPerfMarkerBegin(cmdBuffer, "Magnifier");

    if (m_bOutToSwapChain && !pSwapChain)
    {
        Trace("Warning: MagnifierPS::Draw() is called with nullptr swapchain RTV handle, bOutputsToSwapchain was true during MagnifierPS::OnCreate(). No Draw calls will be issued.");
        return;
    }

    cbHandle_t cbHandle = SetConstantBufferData(params);

    VkRect2D renderArea;
    renderArea.offset.x = 0;
    renderArea.offset.y = 0;
    renderArea.extent.width = params.uImageWidth;
    renderArea.extent.height = params.uImageHeight;
    BeginPass(cmdBuffer, renderArea, pSwapChain);

    mShaderMagnify.Draw(cmdBuffer, &cbHandle, mDescSet);
    EndPass(cmdBuffer);
    SetPerfMarkerEnd(cmdBuffer);
}

void MagnifierPS::UpdatePipelines(VkRenderPass renderPass)
{
    mShaderMagnify.UpdatePipeline(renderPass, nullptr, VK_SAMPLE_COUNT_1_BIT);
}

void MagnifierPS::CompileShaders(StaticBufferPool *pStaticBufferPool, VkFormat outFormat)
{
    mShaderMagnify.OnCreate(
        m_pDevice,
        mRenderPass,
        SHADER_FILE_NAME_MAGNIFIER,
        SHADER_ENTRY_POINT,
        "",
        pStaticBufferPool,
        m_pDynamicBufferRing,
        mDescSetLayout,
        nullptr, VK_SAMPLE_COUNT_1_BIT
    );
}

void MagnifierPS::DestroyShaders()
{
    mShaderMagnify.OnDestroy();
}

void MagnifierPS::InitializeDescriptorSets()
{
    std::vector<VkDescriptorSetLayoutBinding> descSetLayoutBinding(2);
    descSetLayoutBinding[0] = {};
    descSetLayoutBinding[0].binding = 0;
    descSetLayoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descSetLayoutBinding[0].descriptorCount = 1;
    descSetLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descSetLayoutBinding[0].pImmutableSamplers = nullptr;

    descSetLayoutBinding[1] = {};
    descSetLayoutBinding[1].binding = 1;
    descSetLayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descSetLayoutBinding[1].descriptorCount = 1;
    descSetLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descSetLayoutBinding[1].pImmutableSamplers = nullptr;

    m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(
        &descSetLayoutBinding, &mDescSetLayout, &mDescSet);
    m_pDynamicBufferRing->SetDescriptorSet(1, sizeof(PassParameters), mDescSet);
}

void MagnifierPS::UpdateDescriptorSets(VkImageView ImageViewSrc)
{
    // nearest sampler
    {
        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_NEAREST;
        info.minFilter = VK_FILTER_NEAREST;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        info.minLod = -1000;
        info.maxLod = 1000;
        info.maxAnisotropy = 1.0f;
        VK_CHECK_RESULT(vkCreateSampler(m_pDevice->GetDevice(), &info, nullptr, &mSampler))
    }

    constexpr size_t NUM_WRITES = 1;
    constexpr size_t NUM_IMAGES = 1;

    VkDescriptorImageInfo ImgInfo[NUM_IMAGES] = {};
    VkWriteDescriptorSet  SetWrites[NUM_WRITES] = {};

    for (size_t i = 0; i < NUM_IMAGES; ++i) { ImgInfo[i].sampler = mSampler; }
    for (size_t i = 0; i < NUM_WRITES; ++i)
    {
        SetWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        SetWrites[i].dstSet = mDescSet;
        SetWrites[i].descriptorCount = 1;
        SetWrites[i].pImageInfo = ImgInfo + i;
    }

    SetWrites[0].dstBinding = 0;
    SetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ImgInfo[0].imageView = ImageViewSrc;
    ImgInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkUpdateDescriptorSets(m_pDevice->GetDevice(), _countof(SetWrites), SetWrites, 0, nullptr);
}

void MagnifierPS::DestroyDescriptorSets()
{
    m_pResourceViewHeaps->FreeDescriptor(mDescSet);
    vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), mDescSetLayout, nullptr);
}

VkDescriptorBufferInfo MagnifierPS::SetConstantBufferData(const MagnifierPS::PassParameters &params)
{
    KeepMagnifierOnScreen(params);

    // memcpy to cbuffer
    cbHandle_t cbHandle = {};
    uint32_t* pConstMem = nullptr;
    const uint32_t szConstBuff = sizeof(PassParameters);
    void const* pConstBufSrc = static_cast<void const*>(&params);
    m_pDynamicBufferRing->AllocateConstantBuffer(szConstBuff, reinterpret_cast<void**>(&pConstMem), &cbHandle);
    memcpy(pConstMem, pConstBufSrc, szConstBuff);

    return cbHandle;
}

void MagnifierPS::KeepMagnifierOnScreen(const MagnifierPS::PassParameters &params)
{
    const int IMAGE_SIZE[2] = { static_cast<int>(params.uImageWidth), static_cast<int>(params.uImageHeight) };
    const int& W = IMAGE_SIZE[0];
    const int& H = IMAGE_SIZE[1];

    const int radiusInPixelsMagnifier     = static_cast<int>(params.fMagnifierScreenRadius * H);
    const int radiusInPixelsMagnifiedArea = static_cast<int>(params.fMagnifierScreenRadius * H / params.fMagnificationAmount);

    const bool bCirclesAreOverlapping = radiusInPixelsMagnifiedArea + radiusInPixelsMagnifier > std::sqrt(params.iMagnifierOffset[0] * params.iMagnifierOffset[0] + params.iMagnifierOffset[1] * params.iMagnifierOffset[1]);

    if (bCirclesAreOverlapping) // don't let the two circles overlap
    {
        params.iMagnifierOffset[0] = radiusInPixelsMagnifiedArea + radiusInPixelsMagnifier + 1;
        params.iMagnifierOffset[1] = radiusInPixelsMagnifiedArea + radiusInPixelsMagnifier + 1;
    }

    for (int i = 0; i < 2; ++i) // try to move the magnified area to be fully on screen, if possible
    {
        const bool bMagnifierOutOfScreenRegion = 
            params.iMousePos[i] + params.iMagnifierOffset[i] + radiusInPixelsMagnifier > IMAGE_SIZE[i] ||
            params.iMousePos[i] + params.iMagnifierOffset[i] - radiusInPixelsMagnifier < 0;
        if (bMagnifierOutOfScreenRegion)
        {
            if (!(params.iMousePos[i] - params.iMagnifierOffset[i] + radiusInPixelsMagnifier > IMAGE_SIZE[i] || 
                  params.iMousePos[i] - params.iMagnifierOffset[i] - radiusInPixelsMagnifier < 0))
            {
                // flip offset if possible
                params.iMagnifierOffset[i] = -params.iMagnifierOffset[i];
            }
            else
            {
                // otherwise clamp
                if (params.iMousePos[i] + params.iMagnifierOffset[i] + radiusInPixelsMagnifier > IMAGE_SIZE[i])
                    params.iMagnifierOffset[i] = IMAGE_SIZE[i] - params.iMousePos[i] - radiusInPixelsMagnifier;
                if (params.iMousePos[i] + params.iMagnifierOffset[i] - radiusInPixelsMagnifier < 0)
                    params.iMagnifierOffset[i] = -params.iMousePos[i] + radiusInPixelsMagnifier;
            }
        }
    }
}
