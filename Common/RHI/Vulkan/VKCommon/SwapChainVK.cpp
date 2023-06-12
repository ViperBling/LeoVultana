#include "PCHVK.h"
#include "SwapChainVK.h"
#include "ExtFreeSyncHDRVK.h"
#include "ExtDebugUtilsVK.h"
#include "HelperVK.h"

using namespace LeoVultana_VK;

void SwapChain::OnCreate(Device *pDevice, uint32_t numBackBuffers, HWND hWnd)
{
    mHwnd = hWnd;
    m_pDevice = pDevice;
    mBackBufferCount = numBackBuffers;
    mSemaphoreIndex = 0;
    mPrevSemaphoreIndex = 0;

    mPresentQueue = pDevice->GetPresentQueue();

    // Init FSHDR
    FSHDRInit(pDevice->GetDevice(), pDevice->GetSurface(), pDevice->GetPhysicalDevice(), hWnd);

    // Set some safe format to start with
    mDisplayMode = DISPLAYMODE_SDR;
    VkSurfaceFormatKHR surfaceFormat;
    surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    mSwapChainFormat = surfaceFormat;

    VkDevice device = m_pDevice->GetDevice();

    // Create Fence and Semaphore
    mCmdBufferExecutedFences.resize(mBackBufferCount);
    mImageAvailableSemaphores.resize(mBackBufferCount);
    mRenderFinishedSemaphores.resize(mBackBufferCount);
    for (uint32_t i = 0; i < mBackBufferCount; ++i)
    {
        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        // 第一次调用WaitForSwapChain的时候等待的是Fence0，但使用未初始化的Fence1，所以开始时除了0号以外都是非触发状态
        fenceCI.flags = i == 0 ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &mCmdBufferExecutedFences[i]));

        VkSemaphoreCreateInfo imageAcquireSemaphoreCI{};
        imageAcquireSemaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_CHECK_RESULT(vkCreateSemaphore(device, &imageAcquireSemaphoreCI, nullptr, &mImageAvailableSemaphores[i]));
        VK_CHECK_RESULT(vkCreateSemaphore(device, &imageAcquireSemaphoreCI, nullptr, &mRenderFinishedSemaphores[i]));
    }

    // 如果SDR使用了已矫正过Gamma的SwapChain，那么混合也是正确的
    if (mDisplayMode == DISPLAYMODE_SDR)
    {
        assert(mSwapChainFormat.format == VK_FORMAT_R8G8B8A8_UNORM);
        mSwapChainFormat.format = VK_FORMAT_R8G8B8A8_SRGB;
    }
    CreateRenderPass();
}

void SwapChain::OnDestroy()
{
    DestroyRenderPass();

    for (int i = 0; i < mCmdBufferExecutedFences.size(); ++i)
    {
        vkDestroyFence(m_pDevice->GetDevice(), mCmdBufferExecutedFences[i], nullptr);
        vkDestroySemaphore(m_pDevice->GetDevice(), mImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_pDevice->GetDevice(), mRenderFinishedSemaphores[i], nullptr);
    }
}

void SwapChain::OnCreateWindowSizeDependentResources(
    uint32_t dwWidth, uint32_t dwHeight, bool bVSyncOn, DisplayMode displayMode,
    PresentationMode fullScreenMode, bool enableLocalDimming)
{
    bool bIsModeSupported = IsModeSupported(displayMode, fullScreenMode, enableLocalDimming);
    if (!bIsModeSupported)
    {
        assert(!"FSHDR display mode not supported.");
        displayMode = DISPLAYMODE_SDR;
    }
    mDisplayMode = displayMode;
    mSwapChainFormat = FSHDRGetFormat(displayMode);
    m_bVSyncOn = bVSyncOn;

    if (displayMode == DISPLAYMODE_SDR)
    {
        assert(mSwapChainFormat.format == VK_FORMAT_R8G8B8A8_UNORM);
        mSwapChainFormat.format = VK_FORMAT_R8G8B8A8_SRGB;
    }

    VkDevice device = m_pDevice->GetDevice();
    VkPhysicalDevice physicalDevice = m_pDevice->GetPhysicalDevice();
    VkSurfaceKHR surface = m_pDevice->GetSurface();

    DestroyRenderPass();
    CreateRenderPass();

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));

    VkExtent2D swapChainExtent;
    swapChainExtent.width = dwWidth;
    swapChainExtent.height = dwHeight;

    // 宽高肯定同时为nan或同时部位nan
    if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
    {
        // 如果当前尺寸未设置，那么就设为Image请求的大小
        if (swapChainExtent.width < surfaceCapabilities.minImageExtent.width)
            swapChainExtent.width = surfaceCapabilities.minImageExtent.width;
        else if (swapChainExtent.width > surfaceCapabilities.maxImageExtent.width) 
            swapChainExtent.width = surfaceCapabilities.maxImageExtent.width;
        if (swapChainExtent.height < surfaceCapabilities.minImageExtent.height) 
            swapChainExtent.height = surfaceCapabilities.minImageExtent.height;
        else if (swapChainExtent.height > surfaceCapabilities.maxImageExtent.height) 
            swapChainExtent.height = surfaceCapabilities.maxImageExtent.height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        // swapChainExtent = surfCapabilities.currentExtent;
    }
    // Set identity transform
    VkSurfaceTransformFlagBitsKHR preTransform = (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ?
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surfaceCapabilities.currentTransform;

    // 找到一个支持的Alpha模式
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] =
    {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (uint32_t i = 0; i < sizeof(compositeAlphaFlags); i++)
    {
        if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i])
        {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    // Get Present Mode
    uint32_t  presentModeCount;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, &presentModes[0]));

    VkSwapchainCreateInfoKHR swapChainCI{};
    swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCI.pNext = nullptr;
    if (ExtAreFreeSyncHDRExtensionsPresent()) swapChainCI.pNext = GetVkSurfaceFullScreenExclusiveInfoEXT();
    swapChainCI.surface = surface;
    swapChainCI.imageFormat = mSwapChainFormat.format;
    swapChainCI.minImageCount = mBackBufferCount;
    swapChainCI.imageColorSpace = mSwapChainFormat.colorSpace;
    swapChainCI.imageExtent.width = swapChainExtent.width;
    swapChainCI.imageExtent.height = swapChainExtent.height;
    swapChainCI.preTransform = preTransform;
    swapChainCI.compositeAlpha = compositeAlpha;
    swapChainCI.imageArrayLayers = 1;
    swapChainCI.presentMode = m_bVSyncOn ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;  // The FIFO present mode is guaranteed by the spec to be supported
    swapChainCI.oldSwapchain = VK_NULL_HANDLE;
    swapChainCI.clipped = true;
    swapChainCI.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCI.queueFamilyIndexCount = 0;
    swapChainCI.pQueueFamilyIndices = nullptr;
    uint32_t queueFamilyIndices[2] = { m_pDevice->GetGraphicsQueueFamilyIndex(), m_pDevice->GetPresentQueueFamilyIndex() };
    if (queueFamilyIndices[0] != queueFamilyIndices[1])
    {
        // If the graphics and present queues are from different queue families,
        // we either have to explicitly transfer ownership of images between the
        // queues, or we have to create the swapchain with imageSharingMode
        // as VK_SHARING_MODE_CONCURRENT
        swapChainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCI.queueFamilyIndexCount = 2;
        swapChainCI.pQueueFamilyIndices = queueFamilyIndices;
    }
    VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapChainCI, nullptr, &mSwapChain))

    // Get capabilities is only for FS HDR
    if (displayMode == DISPLAYMODE_FSHDR_Gamma22 || displayMode == DISPLAYMODE_FSHDR_SCRGB)
    {
        FSHDRGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, surface, nullptr);
    }
    FSHDRSetDisplayMode(displayMode, mSwapChain);

    // we are querying the swapchain count so the next call doesn't generate a validation warning
    uint32_t backBufferCount;
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, mSwapChain, &backBufferCount, nullptr))

    assert(backBufferCount == mBackBufferCount);

    mImages.resize(mBackBufferCount);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, mSwapChain, &mBackBufferCount, mImages.data()))

    CreateRTV();
    CreateFrameBuffers(dwWidth, dwHeight);

    mImageIndex = 0;
}

void SwapChain::OnDestroyWindowSizeDependentResources()
{
    DestroyRenderPass();
    DestroyFrameBuffers();
    DestroyRTV();
    vkDestroySwapchainKHR(m_pDevice->GetDevice(), mSwapChain, nullptr);
}

void SwapChain::SetFullScreen(bool fullscreen)
{
    if (ExtAreFSEExtensionsPresent()) FSHDRSetFullscreenState(fullscreen, mSwapChain);
}

bool SwapChain::IsModeSupported(
    DisplayMode displayMode,
    PresentationMode fullScreenMode,
    bool enableLocalDimming)
{
    std::vector<DisplayMode> displayModeAvailable;
    EnumerateDisplayMode(&displayModeAvailable, nullptr, displayMode != DISPLAYMODE_SDR, fullScreenMode, enableLocalDimming);
    return std::find(displayModeAvailable.begin(), displayModeAvailable.end(), displayMode) != displayModeAvailable.end();
}

void SwapChain::EnumerateDisplayMode(
    std::vector<DisplayMode> *pModes,
    std::vector<const char *> *pNames,
    bool includeFreeSyncHDR, PresentationMode fullScreenMode, bool enableLocalDimming)
{
    FSHDREnumerateDisplayModes(pModes, includeFreeSyncHDR, fullScreenMode, enableLocalDimming);

    if (pNames)
    {
        pNames->clear();
        for (auto mode : *pModes)
            pNames->push_back(FSHDRGetDisplayModeString(mode));
    }
}

void SwapChain::GetSemaphores(
    VkSemaphore *pImageAvailableSemaphore,
    VkSemaphore *pRenderFinishedSemaphores,
    VkFence *pCmdBufferExecuteFences)
{
    *pImageAvailableSemaphore = mImageAvailableSemaphores[mPrevSemaphoreIndex];
    *pRenderFinishedSemaphores = mRenderFinishedSemaphores[mSemaphoreIndex];
    *pCmdBufferExecuteFences = mCmdBufferExecutedFences[mSemaphoreIndex];
}

VkResult SwapChain::Present()
{
    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = nullptr;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &(mRenderFinishedSemaphores[mSemaphoreIndex]);
    present.swapchainCount = 1;
    present.pSwapchains = &mSwapChain;
    present.pImageIndices = &mImageIndex;
    present.pResults = nullptr;

    return vkQueuePresentKHR(mPresentQueue, &present);
}

uint32_t SwapChain::WaitForSwapChain()
{
    vkAcquireNextImageKHR(m_pDevice->GetDevice(), mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mSemaphoreIndex], VK_NULL_HANDLE, &mImageIndex);

    mPrevSemaphoreIndex = mSemaphoreIndex;
    mSemaphoreIndex++;
    if (mSemaphoreIndex >= mBackBufferCount) mSemaphoreIndex = 0;

    vkWaitForFences(m_pDevice->GetDevice(), 1, &mCmdBufferExecutedFences[mPrevSemaphoreIndex], VK_TRUE, UINT64_MAX);
    vkResetFences(m_pDevice->GetDevice(), 1, &mCmdBufferExecutedFences[mPrevSemaphoreIndex]);

    return mImageIndex;
}

VkImage SwapChain::GetCurrentBackBuffer()
{
    return mImages[mImageIndex];
}

VkImageView SwapChain::GetCurrentBackBufferRTV()
{
    return mImageViews[mImageIndex];
}

void SwapChain::CreateRTV()
{
    mImageViews.resize(mImages.size());
    for (uint32_t i = 0; i < mImages.size(); i++)
    {
        VkImageViewCreateInfo imageViewCI{};
        imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCI.pNext = nullptr;
        imageViewCI.format = mSwapChainFormat.format;
        imageViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
        imageViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
        imageViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
        imageViewCI.components.a = VK_COMPONENT_SWIZZLE_A;
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCI.subresourceRange.baseMipLevel = 0;
        imageViewCI.subresourceRange.levelCount = 1;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.layerCount = 1;
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCI.flags = 0;
        imageViewCI.image = mImages[i];

        VK_CHECK_RESULT(vkCreateImageView(m_pDevice->GetDevice(), &imageViewCI, nullptr, &mImageViews[i]));

        SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)mImageViews[i], "SwapChain");
    }
}

void SwapChain::DestroyRTV()
{
    for (uint32_t i = 0; i < mImageViews.size(); i++)
        vkDestroyImageView(m_pDevice->GetDevice(), mImageViews[i], nullptr);
}

void SwapChain::CreateRenderPass()
{
    // color RT
    VkAttachmentDescription colorAttachments[1];
    colorAttachments[0].format = mSwapChainFormat.format;
    colorAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachments[0].flags = 0;

    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkSubpassDependency subpassDependency{};
    subpassDependency.dependencyFlags = 0;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;

    VkRenderPassCreateInfo renderPassCI = {};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.pNext = nullptr;
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = colorAttachments;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpass;
    renderPassCI.dependencyCount = 1;
    renderPassCI.pDependencies = &subpassDependency;

    VK_CHECK_RESULT(vkCreateRenderPass(m_pDevice->GetDevice(), &renderPassCI, nullptr, &mRenderPassSwapChain));
}

void SwapChain::DestroyRenderPass()
{
    if (mRenderPassSwapChain != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_pDevice->GetDevice(), mRenderPassSwapChain, nullptr);
        mRenderPassSwapChain = VK_NULL_HANDLE;
    }
}

void SwapChain::CreateFrameBuffers(uint32_t dwWidth, uint32_t dwHeight)
{
    mFrameBuffers.resize(mImageViews.size());
    for (uint32_t i = 0; i < mImageViews.size(); i++)
    {
        VkImageView attachments[] = { mImageViews[i] };

        VkFramebufferCreateInfo frameBufferCI{};
        frameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCI.pNext = nullptr;
        frameBufferCI.renderPass = mRenderPassSwapChain;
        frameBufferCI.attachmentCount = 1;
        frameBufferCI.pAttachments = attachments;
        frameBufferCI.width = dwWidth;
        frameBufferCI.height = dwHeight;
        frameBufferCI.layers = 1;

        VK_CHECK_RESULT(vkCreateFramebuffer(m_pDevice->GetDevice(), &frameBufferCI, nullptr, &mFrameBuffers[i]));

        SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)mFrameBuffers[i], "SwapChainFrameBuffer");
    }
}

void SwapChain::DestroyFrameBuffers()
{
    for (uint32_t i = 0; i < mFrameBuffers.size(); i++)
        vkDestroyFramebuffer(m_pDevice->GetDevice(), mFrameBuffers[i], nullptr);
}
