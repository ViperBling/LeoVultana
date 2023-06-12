#include "PCHVK.h"
#include "TextureVK.h"
#include "HelperVK.h"
#include "Misc.h"
#include "DXGIFormatHelper.h"
#include "ExtDebugUtilsVK.h"

using namespace LeoVultana_VK;

namespace LeoVultana_VK
{
    VkFormat TranslateDXGIFormatIntoVulkan(DXGI_FORMAT format)
    {
        switch (format)
        {
            case DXGI_FORMAT_B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
            case DXGI_FORMAT_BC1_UNORM:         return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            case DXGI_FORMAT_BC1_UNORM_SRGB:    return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            case DXGI_FORMAT_BC2_UNORM:         return VK_FORMAT_BC2_UNORM_BLOCK;
            case DXGI_FORMAT_BC2_UNORM_SRGB:    return VK_FORMAT_BC2_SRGB_BLOCK;
            case DXGI_FORMAT_BC3_UNORM:         return VK_FORMAT_BC3_UNORM_BLOCK;
            case DXGI_FORMAT_BC3_UNORM_SRGB:    return VK_FORMAT_BC3_SRGB_BLOCK;
            case DXGI_FORMAT_BC4_UNORM:         return VK_FORMAT_BC4_UNORM_BLOCK;
            case DXGI_FORMAT_BC4_SNORM:         return VK_FORMAT_BC4_UNORM_BLOCK;
            case DXGI_FORMAT_BC5_UNORM:         return VK_FORMAT_BC5_UNORM_BLOCK;
            case DXGI_FORMAT_BC5_SNORM:         return VK_FORMAT_BC5_UNORM_BLOCK;
            case DXGI_FORMAT_B5G6R5_UNORM:      return VK_FORMAT_B5G6R5_UNORM_PACK16;
            case DXGI_FORMAT_B5G5R5A1_UNORM:    return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
            case DXGI_FORMAT_BC6H_UF16:         return VK_FORMAT_BC6H_UFLOAT_BLOCK;
            case DXGI_FORMAT_BC6H_SF16:         return VK_FORMAT_BC6H_SFLOAT_BLOCK;
            case DXGI_FORMAT_BC7_UNORM:         return VK_FORMAT_BC7_UNORM_BLOCK;
            case DXGI_FORMAT_BC7_UNORM_SRGB:    return VK_FORMAT_BC7_SRGB_BLOCK;
            case DXGI_FORMAT_R10G10B10A2_UNORM: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
            case DXGI_FORMAT_R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case DXGI_FORMAT_R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case DXGI_FORMAT_A8_UNORM:          return VK_FORMAT_R8_UNORM;
            default: assert(false);  return VK_FORMAT_UNDEFINED;
        }
    }
}

void Texture::OnDestroy()
{
#ifdef USE_VMA
    if (mResource != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_pDevice->GetAllocator(), mResource, mImageAlloc);
        mResource = VK_NULL_HANDLE;
    }
#else
    if (mDeviceMemory != VK_NULL_HANDLE)
    {
        vkDestroyImage(m_pDevice->GetDevice(), mResource, nullptr);
        vkFreeMemory(m_pDevice->GetDevice(), mDeviceMemory, nullptr);
        mDeviceMemory = VK_NULL_HANDLE;
    }
#endif
}

INT32 Texture::Init(Device *pDevice, VkImageCreateInfo *pCreateInfo, const char *name)
{
    m_pDevice = pDevice;
    mHeader.mipMapCount = pCreateInfo->mipLevels;
    mHeader.width = pCreateInfo->extent.width;
    mHeader.height = pCreateInfo->extent.height;
    mHeader.depth = pCreateInfo->extent.depth;
    mHeader.arraySize = pCreateInfo->arrayLayers;
    mFormat = pCreateInfo->format;
    if (name) mName = name;

#ifdef USE_VMA
    VmaAllocationCreateInfo imageAllocateCI{};
    imageAllocateCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocateCI.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    imageAllocateCI.pUserData = (void*)mName.c_str();
    VmaAllocationInfo gpuImageAI{};
    VK_CHECK_RESULT(vmaCreateImage(
        m_pDevice->GetAllocator(), pCreateInfo,
        &imageAllocateCI, &mResource, &mImageAlloc, &gpuImageAI));
    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_IMAGE, (uint64_t)mResource, mName.c_str());
#else
    // Create image
    VK_CHECK_RESULT(vkCreateImage(m_pDevice->GetDevice(), pCreateInfo, nullptr, &mResource));
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(m_pDevice->GetDevice(), mResource, &memoryRequirements);
    VkMemoryAllocateInfo imageMemoryAI{};
    imageMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    imageMemoryAI.pNext = nullptr;
    imageMemoryAI.allocationSize = 0;
    imageMemoryAI.allocationSize = memoryRequirements.size;
    imageMemoryAI.memoryTypeIndex = 0;

    bool pass = MemoryTypeFromProperties(
        m_pDevice->GetPhysicalDeviceMemoryProperties(),
        memoryRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &imageMemoryAI.memoryTypeIndex);
    assert(pass);
    // Allocate Memory
    VK_CHECK_RESULT(vkAllocateMemory(m_pDevice->GetDevice(), &imageMemoryAI, nullptr, &mDeviceMemory));
    VK_CHECK_RESULT(vkBindImageMemory(m_pDevice->GetDevice(), mResource, mDeviceMemory, 0));
#endif
    return 0;
}

INT32 Texture::InitRenderTarget(
    Device *pDevice, uint32_t width, uint32_t height, VkFormat format,
    VkSampleCountFlagBits msaa, VkImageUsageFlags usage,
    bool bUAV, const char *name,
    VkImageCreateFlagBits flags)
{
    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.pNext = nullptr;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent.width = width;
    imageCI.extent.height = height;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = msaa;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCI.queueFamilyIndexCount = 0;
    imageCI.pQueueFamilyIndices = nullptr;
    imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCI.usage = usage;
    imageCI.flags = flags;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;   // VK_IMAGE_TILING_LINEAR should never be used and will never be faster

    return Init(pDevice, &imageCI, name);
}

INT32 Texture::InitDepthStencil(
    Device *pDevice, uint32_t width, uint32_t height, VkFormat format,
    VkSampleCountFlagBits msaa, const char *name)
{
    VkImageCreateInfo imageCI = {};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.pNext = nullptr;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = format;
    imageCI.extent.width = width;
    imageCI.extent.height = height;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = msaa;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCI.queueFamilyIndexCount = 0;
    imageCI.pQueueFamilyIndices = nullptr;
    imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; //TODO
    imageCI.flags = 0;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;   // VK_IMAGE_TILING_LINEAR should never be used and will never be faster

    return Init(pDevice, &imageCI, name);
}

bool Texture::InitFromFile(
    Device *pDevice, UploadHeap *pUploadHeap,
    const char *szFilename, bool useSRGB,
    VkImageUsageFlags usageFlags, float cutOff)
{
    m_pDevice = pDevice;
    assert(mResource == VK_NULL_HANDLE);

    ImgLoader* img = CreateImageLoader(szFilename);
    bool result = img->Load(szFilename, cutOff, &mHeader);
    if (result)
    {
        mResource = CreateTextureCommitted(pDevice, pUploadHeap, szFilename, useSRGB, usageFlags);
        LoadAndUpload(pDevice, pUploadHeap, img, mResource);
    }
    else
    {
        Trace("Error loading texture from file: %s", szFilename);
        assert(result && "Could not load requested file. Please make sure it exists on disk.");
    }

    delete(img);

    return result;
}

bool Texture::InitFromData(
    Device *pDevice, UploadHeap &uploadHeap,
    const IMG_INFO &header, const void *data,
    const char *name)
{
    assert(!mResource && !m_pDevice);
    assert(header.arraySize == 1 && header.mipMapCount == 1);

    m_pDevice = pDevice;
    mHeader = header;

    mResource = CreateTextureCommitted(m_pDevice, &uploadHeap, name, false);

    // Upload Image
    VkImageMemoryBarrier copy_barrier = {};
    copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copy_barrier.image = mResource;
    copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_barrier.subresourceRange.baseMipLevel = 0;
    copy_barrier.subresourceRange.levelCount = mHeader.mipMapCount;
    copy_barrier.subresourceRange.layerCount = mHeader.arraySize;
    vkCmdPipelineBarrier(
        uploadHeap.GetCommandList(),
        VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr,
        0, nullptr,
        1, &copy_barrier);

    //compute pixel size
    UINT32 bytePP = mHeader.bitCount / 8;
    if ((mHeader.format >= DXGI_FORMAT_BC1_TYPELESS) && (mHeader.format <= DXGI_FORMAT_BC5_SNORM))
    {
        bytePP = (UINT32)GetPixelByteSize((DXGI_FORMAT)mHeader.format);
    }

    UINT8* pixels = nullptr;
    UINT64 UplHeapSize = mHeader.width * mHeader.height * 4;
    pixels = uploadHeap.SubAllocate(UplHeapSize, 512);
    assert(pixels != nullptr);

    CopyMemory( pixels, data, mHeader.width * mHeader.height * bytePP );

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.mipLevel = 0;
    region.imageExtent.width = mHeader.width;
    region.imageExtent.height = mHeader.height;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(
        uploadHeap.GetCommandList(), uploadHeap.GetResource(),
        mResource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &region);

    // prepare to shader read
    VkImageMemoryBarrier use_barrier = {};
    use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    use_barrier.image = mResource;
    use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    use_barrier.subresourceRange.levelCount = mHeader.mipMapCount;
    use_barrier.subresourceRange.layerCount = mHeader.arraySize;
    vkCmdPipelineBarrier(
        uploadHeap.GetCommandList(), VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr,
        0, nullptr,
        1, &use_barrier);

    return true;
}

void Texture::CreateRTV(VkImageView *pImageView, int mipLevel, VkFormat format)
{
    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.image = mResource;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    if (mHeader.arraySize > 1)
    {
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        imageViewCI.subresourceRange.layerCount = mHeader.arraySize;
    }
    else
    {
        imageViewCI.subresourceRange.layerCount = 1;
    }

    if (format == VK_FORMAT_UNDEFINED) imageViewCI.format = mFormat;
    else imageViewCI.format = format;

    if (mFormat == VK_FORMAT_D32_SFLOAT)
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    std::string resourceName = mName;

    if (mipLevel == -1)
    {
        imageViewCI.subresourceRange.baseMipLevel = 0;
        imageViewCI.subresourceRange.levelCount = mHeader.mipMapCount;
    }
    else
    {
        imageViewCI.subresourceRange.baseMipLevel = mipLevel;
        imageViewCI.subresourceRange.levelCount = 1;
        resourceName += std::to_string(mipLevel);
    }

    imageViewCI.subresourceRange.baseArrayLayer = 0;
    VK_CHECK_RESULT(vkCreateImageView(m_pDevice->GetDevice(), &imageViewCI, nullptr, pImageView));

    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, resourceName.c_str());
}

void Texture::CreateSRV(VkImageView *pImageView, int mipLevel)
{
    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.image = mResource;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    if (mHeader.arraySize > 1)
    {
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        imageViewCI.subresourceRange.layerCount = mHeader.arraySize;
    }
    else
    {
        imageViewCI.subresourceRange.layerCount = 1;
    }
    imageViewCI.format = mFormat;
    if (mFormat == VK_FORMAT_D32_SFLOAT)
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (mipLevel == -1)
    {
        imageViewCI.subresourceRange.baseMipLevel = 0;
        imageViewCI.subresourceRange.levelCount = mHeader.mipMapCount;
    }
    else
    {
        imageViewCI.subresourceRange.baseMipLevel = mipLevel;
        imageViewCI.subresourceRange.levelCount = 1;
    }
    imageViewCI.subresourceRange.baseArrayLayer = 0;

    VK_CHECK_RESULT(vkCreateImageView(m_pDevice->GetDevice(), &imageViewCI, nullptr, pImageView));

    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, mName.c_str());
}

void Texture::CreateDSV(VkImageView *pImageView)
{
    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.pNext = nullptr;
    imageViewCI.image = mResource;
    imageViewCI.format = mFormat;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;

    if (mFormat == VK_FORMAT_D16_UNORM_S8_UINT || mFormat == VK_FORMAT_D24_UNORM_S8_UINT || mFormat == VK_FORMAT_D32_SFLOAT_S8_UINT)
    {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    mHeader.mipMapCount = 1;

    VK_CHECK_RESULT(vkCreateImageView(m_pDevice->GetDevice(), &imageViewCI, nullptr, pImageView));

    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, mName.c_str());
}

void Texture::CreateCubeSRV(VkImageView *pImageView)
{
    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.image = mResource;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    imageViewCI.format = mFormat;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = mHeader.mipMapCount;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = mHeader.arraySize;

    VK_CHECK_RESULT(vkCreateImageView(m_pDevice->GetDevice(), &imageViewCI, nullptr, pImageView));

    SetResourceName(m_pDevice->GetDevice(), VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*pImageView, mName.c_str());
}

VkImage Texture::CreateTextureCommitted(
    Device *pDevice, UploadHeap *pUploadHeap,
    const char *pName, bool useSRGB,
    VkImageUsageFlags usageFlags)
{
    VkImageCreateInfo imageCI{};

    if ( pName )
        mName = pName;

    if (useSRGB && ((usageFlags & VK_IMAGE_USAGE_STORAGE_BIT) != 0))
    {
        // the storage bit is not supported for srgb formats
        // we can still use the srgb format on an image view if the access is read-only
        // for write access, we need to use an image view with unorm format
        // this is ok as srgb and unorm formats are compatible with each other
        VkImageFormatListCreateInfo formatListInfo = {};
        formatListInfo.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
        formatListInfo.viewFormatCount = 2;
        VkFormat list[2];
        list[0] = TranslateDXGIFormatIntoVulkan(mHeader.format);
        list[1] = TranslateDXGIFormatIntoVulkan(SetFormatGamma(mHeader.format, useSRGB));
        formatListInfo.pViewFormats = list;

        imageCI.pNext = &formatListInfo;
    }
    else
    {
        mHeader.format = SetFormatGamma(mHeader.format, useSRGB);
    }

    mFormat = TranslateDXGIFormatIntoVulkan((DXGI_FORMAT)mHeader.format);

    VkImage tex;

    // Create the Image:
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = mFormat;
    imageCI.extent.width = mHeader.width;
    imageCI.extent.height = mHeader.height;
    imageCI.extent.depth = 1;
    imageCI.mipLevels = mHeader.mipMapCount;
    imageCI.arrayLayers = mHeader.arraySize;
    if (mHeader.arraySize == 6)
        imageCI.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | usageFlags;
    imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // allocate memory and bind the image to it
#ifdef USE_VMA
    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    imageAllocCreateInfo.pUserData = (void*)mName.c_str();
    VmaAllocationInfo gpuImageAllocInfo = {};
    VK_CHECK_RESULT(vmaCreateImage(
        pDevice->GetAllocator(), &imageCI,
        &imageAllocCreateInfo, &tex,
        &mImageAlloc, &gpuImageAllocInfo));

    SetResourceName(pDevice->GetDevice(), VK_OBJECT_TYPE_IMAGE, (uint64_t)tex, mName.c_str());
#else
    VK_CHECK_RESULT(vkCreateImage(pDevice->GetDevice(), &imageCI, nullptr, &tex));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(pDevice->GetDevice(), tex, &memReqs);

    VkMemoryAllocateInfo memAI = {};
    memAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAI.allocationSize = memReqs.size;
    memAI.memoryTypeIndex = 0;

    bool pass = MemoryTypeFromProperties(pDevice->GetPhysicalDeviceMemoryProperties(), memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &memAI.memoryTypeIndex);
    assert(pass && "No mappable, coherent memory");

    VK_CHECK_RESULT(vkAllocateMemory(pDevice->GetDevice(), &memAI, nullptr, &mDeviceMemory));
    VK_CHECK_RESULT(vkBindImageMemory(pDevice->GetDevice(), tex, mDeviceMemory, 0));
#endif

    return tex;
}

void Texture::LoadAndUpload(Device *pDevice, UploadHeap *pUploadHeap, ImgLoader *pDds, VkImage pTexture2D)
{
    // Upload Image
    VkImageMemoryBarrier copyBarrier = {};
    copyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    copyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    copyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyBarrier.image = pTexture2D;
    copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyBarrier.subresourceRange.baseMipLevel = 0;
    copyBarrier.subresourceRange.levelCount = mHeader.mipMapCount;
    copyBarrier.subresourceRange.layerCount = mHeader.arraySize;
    pUploadHeap->AddPreBarrier(copyBarrier);

    //compute pixel size
    auto bytesPerPixel = (UINT32)GetPixelByteSize((DXGI_FORMAT)mHeader.format); // note that bytesPerPixel in BC formats is treated as bytesPerBlock
    UINT32 pixelsPerBlock = 1;
    if (IsBCFormat(mHeader.format))
    {
        pixelsPerBlock = 4 * 4; // BC formats have 4*4 pixels per block
    }

    for (uint32_t a = 0; a < mHeader.arraySize; a++)
    {
        // copy all the mip slices into the offsets specified by the footprint structure
        for (uint32_t mip = 0; mip < mHeader.mipMapCount; mip++)
        {
            uint32_t dwWidth = std::max<uint32_t>(mHeader.width >> mip, 1);
            uint32_t dwHeight = std::max<uint32_t>(mHeader.height >> mip, 1);

            UINT64 UplHeapSize = (dwWidth * dwHeight * bytesPerPixel) / pixelsPerBlock;
            UINT8 *pixels = pUploadHeap->BeginSubAllocate(SIZE_T(UplHeapSize), 512);

            if (pixels == nullptr)
            {
                // oh! We ran out of mem in the upload heap, flush it and try allocating mem from it again
                pUploadHeap->FlushAndFinish(true);
                pixels = pUploadHeap->SubAllocate(SIZE_T(UplHeapSize), 512);
                assert(pixels != nullptr);
            }

            auto offset = uint32_t(pixels - pUploadHeap->BasePtr());
            pDds->CopyPixels(pixels, (dwWidth * bytesPerPixel) / pixelsPerBlock, (dwWidth * bytesPerPixel) / pixelsPerBlock, dwHeight);
            pUploadHeap->EndSubAllocate();

            VkBufferImageCopy region = {};
            region.bufferOffset = offset;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageSubresource.baseArrayLayer = a;
            region.imageSubresource.mipLevel = mip;
            region.imageExtent.width = dwWidth;
            region.imageExtent.height = dwHeight;
            region.imageExtent.depth = 1;
            pUploadHeap->AddCopy(pTexture2D, region);
        }
    }

    // prepare to shader read
    VkImageMemoryBarrier use_barrier = {};
    use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    use_barrier.image = pTexture2D;
    use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    use_barrier.subresourceRange.levelCount = mHeader.mipMapCount;
    use_barrier.subresourceRange.layerCount = mHeader.arraySize;
    pUploadHeap->AddPostBarrier(use_barrier);
}

bool Texture::IsCubeMap() const
{
    return mHeader.arraySize == 6;
}