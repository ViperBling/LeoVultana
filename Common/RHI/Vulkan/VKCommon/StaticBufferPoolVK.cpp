#include "PCHVK.h"
#include "StaticBufferPoolVK.h"
#include "Misc.h"
#include "HelperVK.h"
#include "ExtDebugUtilsVK.h"

using namespace LeoVultana_VK;


void StaticBufferPool::OnCreate(Device *pDevice, uint32_t totalMemSize, bool bUseVidMem, const char *name)
{
    m_pDevice = pDevice;

    mTotalMemSize = totalMemSize;
    mMemOffset = 0;
    m_pData = nullptr;
    m_bUseVidMem = bUseVidMem;

    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = mTotalMemSize;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (bUseVidMem) bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

#ifdef USE_VMA
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    allocInfo.pUserData = (void*)name;
    VK_CHECK_RESULT(vmaCreateBuffer(
        pDevice->GetAllocator(),
        &bufferInfo, &allocInfo,
        &mBuffer, &mBufferAlloc, nullptr));
    
    SetResourceName(pDevice->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)mBuffer, "StaticBufferPool (sys mem)");
    VK_CHECK_RESULT(vmaMapMemory(pDevice->GetAllocator(), mBufferAlloc, (void **)&m_pData));

#else
    // create the buffer, allocate it in SYSTEM memory, bind it and map it
    VkBufferCreateInfo deviceBufferCI{};
    deviceBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    deviceBufferCI.pNext = nullptr;
    deviceBufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (bUseVidMem)
        deviceBufferCI.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    deviceBufferCI.size = mTotalMemSize;
    deviceBufferCI.queueFamilyIndexCount = 0;
    deviceBufferCI.pQueueFamilyIndices = nullptr;
    deviceBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    deviceBufferCI.flags = 0;
    VK_CHECK_RESULT(vkCreateBuffer(m_pDevice->GetDevice(), &deviceBufferCI, nullptr, &mBuffer));

    // allocate the buffer in system memory
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_pDevice->GetDevice(), mBuffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    allocInfo.allocationSize = memReqs.size;

    bool pass = MemoryTypeFromProperties(
        m_pDevice->GetPhysicalDeviceMemoryProperties(),
        memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &allocInfo.memoryTypeIndex);
    assert(pass && "No mappable, coherent memory");

    VK_CHECK_RESULT(vkAllocateMemory(m_pDevice->GetDevice(), &allocInfo, nullptr, &mDeviceMemory));

    // bind buffer
    VK_CHECK_RESULT(vkBindBufferMemory(m_pDevice->GetDevice(), mBuffer, mDeviceMemory, 0));

    // Map it and leave it mapped. This is fine for Win10 and Win7.
    VK_CHECK_RESULT(vkMapMemory(m_pDevice->GetDevice(), mDeviceMemory, 0, memReqs.size, 0, (void **)&m_pData));
#endif

    if (m_bUseVidMem)
    {
#ifdef USE_VMA
        VkBufferCreateInfo bufferVidCI = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferVidCI.size = mTotalMemSize;
        bufferVidCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocVidCI{};
        allocVidCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocVidCI.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        allocVidCI.pUserData = (void*)name;
        VK_CHECK_RESULT(vmaCreateBuffer(pDevice->GetAllocator(), &bufferVidCI, &allocVidCI, &mBufferVid, &mBufferAllocVid, nullptr));
        
        SetResourceName(pDevice->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)mBuffer, "StaticBufferPool (vid mem)");

#else
        // create the buffer, allocate it in VIDEO memory and bind it
        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.pNext = nullptr;
        bufferCI.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferCI.size = mTotalMemSize;
        bufferCI.queueFamilyIndexCount = 0;
        bufferCI.pQueueFamilyIndices = nullptr;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCI.flags = 0;
        VK_CHECK_RESULT(vkCreateBuffer(m_pDevice->GetDevice(), &bufferCI, nullptr, &mBufferVid));

        // allocate the buffer in VIDEO memory

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(m_pDevice->GetDevice(), mBufferVid, &memReqs);

        VkMemoryAllocateInfo allocCI{};
        allocCI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocCI.pNext = nullptr;
        allocCI.memoryTypeIndex = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        allocCI.allocationSize = memReqs.size;

        bool pass = MemoryTypeFromProperties(
            m_pDevice->GetPhysicalDeviceMemoryProperties(),
            memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocCI.memoryTypeIndex);
        assert(pass && "No mappable, coherent memory");

        VK_CHECK_RESULT(vkAllocateMemory(m_pDevice->GetDevice(), &allocCI, nullptr, &mDeviceMemoryVid));

        // bind buffer
        VK_CHECK_RESULT(vkBindBufferMemory(m_pDevice->GetDevice(), mBufferVid, mDeviceMemoryVid, 0));
#endif
    }
}

void StaticBufferPool::OnDestroy()
{
    if (m_bUseVidMem)
    {
#ifdef USE_VMA
        vmaDestroyBuffer(m_pDevice->GetAllocator(), mBufferVid, mBufferAllocVid);
#else
        vkFreeMemory(m_pDevice->GetDevice(), mDeviceMemoryVid, nullptr);
        vkDestroyBuffer(m_pDevice->GetDevice(), mBufferVid, nullptr);
#endif
    }

    if (mBuffer != VK_NULL_HANDLE)
    {
#ifdef USE_VMA
        vmaUnmapMemory(m_pDevice->GetAllocator(), mBufferAlloc);
        vmaDestroyBuffer(m_pDevice->GetAllocator(), mBuffer, mBufferAlloc);
#else
        vkUnmapMemory(m_pDevice->GetDevice(), mDeviceMemory);
        vkFreeMemory(m_pDevice->GetDevice(), mDeviceMemory, nullptr);
        vkDestroyBuffer(m_pDevice->GetDevice(), mBuffer, nullptr);
#endif
        mBuffer = VK_NULL_HANDLE;
    }
}

bool StaticBufferPool::AllocateBuffer(
    uint32_t numberOfVertices,
    uint32_t strideInBytes,
    void **pData,
    VkDescriptorBufferInfo *pOut)
{
    std::lock_guard<std::mutex> lock(mMutex);

    uint32_t size = AlignUp(numberOfVertices * strideInBytes, 256u);
    assert(mMemOffset + size < mTotalMemSize);
    if (mMemOffset + size >= mTotalMemSize) return false;

    *pData = (void *)(m_pData + mMemOffset);

    pOut->buffer = m_bUseVidMem ? mBufferVid : mBuffer;
    pOut->offset = mMemOffset;
    pOut->range = size;

    mMemOffset += size;

    return true;
}

bool StaticBufferPool::AllocateBuffer(
    uint32_t numberOfIndices,
    uint32_t strideInBytes,
    const void *pInitData,
    VkDescriptorBufferInfo *pOut)
{
    void *pData;
    if (AllocateBuffer(numberOfIndices, strideInBytes, &pData, pOut))
    {
        memcpy(pData, pInitData, numberOfIndices * strideInBytes);
        return true;
    }
    return false;
}

void StaticBufferPool::UploadData(VkCommandBuffer cmdBuffer)
{
    VkBufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = mTotalMemSize;

    vkCmdCopyBuffer(cmdBuffer, mBuffer, mBufferVid, 1, &region);
}

void StaticBufferPool::FreeUploadHeap()
{
    if (m_bUseVidMem)
    {
        assert(mBuffer != VK_NULL_HANDLE);
#ifdef USE_VMA
        vmaUnmapMemory(m_pDevice->GetAllocator(), mBufferAlloc);
        vmaDestroyBuffer(m_pDevice->GetAllocator(), mBuffer, mBufferAlloc);
#else
        //release upload heap
        vkUnmapMemory(m_pDevice->GetDevice(), mDeviceMemory);
        vkFreeMemory(m_pDevice->GetDevice(), mDeviceMemory, nullptr);
        mDeviceMemory = VK_NULL_HANDLE;
        vkDestroyBuffer(m_pDevice->GetDevice(), mBuffer, nullptr);
#endif
        mBuffer = VK_NULL_HANDLE;
    }
}
