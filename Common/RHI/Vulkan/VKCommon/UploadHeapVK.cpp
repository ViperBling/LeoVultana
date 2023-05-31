#include "PCHVK.h"
#include "UploadHeapVK.h"
#include "HelperVK.h"
#include "Misc.h"

using namespace LeoVultana_VK;

void UploadHeap::OnCreate(Device *pDevice, SIZE_T uSize)
{
    m_pDevice = pDevice;

    // Create command list and allocators
    VkCommandPoolCreateInfo cmdPoolCI{};
    cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCI.queueFamilyIndex = m_pDevice->GetGraphicsQueueFamilyIndex();
    cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(m_pDevice->GetDevice(), &cmdPoolCI, nullptr, &mCmdPool));

    VkCommandBufferAllocateInfo cmdBufferAI{};
    cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAI.pNext = nullptr;
    cmdBufferAI.commandPool = mCmdPool;
    cmdBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAI.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_pDevice->GetDevice(), &cmdBufferAI, &mCmdBuffer));

    // Create buffer to suballocate
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = uSize;
    bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer(m_pDevice->GetDevice(), &bufferCI, nullptr, &mBuffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_pDevice->GetDevice(), mBuffer, &memRequirements);

    VkMemoryAllocateInfo memoryAI{};
    memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAI.allocationSize = memRequirements.size;
    memoryAI.memoryTypeIndex = 0;

    bool pass = MemoryTypeFromProperties(
        m_pDevice->GetPhysicalDeviceMemoryProperties(),
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &memoryAI.memoryTypeIndex);
    assert(pass && "No mappable, coherent memory");

    VK_CHECK_RESULT(vkAllocateMemory(m_pDevice->GetDevice(), &memoryAI, nullptr, &mDeviceMemory));
    VK_CHECK_RESULT(vkBindBufferMemory(m_pDevice->GetDevice(), mBuffer, mDeviceMemory, 0));
    VK_CHECK_RESULT(vkMapMemory(m_pDevice->GetDevice(), mDeviceMemory, 0, memRequirements.size, 0, (void **)&m_pDataBegin));

    m_pDataCur = m_pDataBegin;
    m_pDataEnd = m_pDataBegin + memRequirements.size;

    // Create fence
    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreateFence(m_pDevice->GetDevice(), &fenceCI, nullptr, &mFence));

    // Begin Command Buffer
    VkCommandBufferBeginInfo cmdBufferBI{};
    cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK_RESULT(vkBeginCommandBuffer(mCmdBuffer, &cmdBufferBI));
}

void UploadHeap::OnDestroy()
{
    vkDestroyBuffer(m_pDevice->GetDevice(), mBuffer, nullptr);
    vkUnmapMemory(m_pDevice->GetDevice(), mDeviceMemory);
    vkFreeMemory(m_pDevice->GetDevice(), mDeviceMemory, nullptr);

    vkFreeCommandBuffers(m_pDevice->GetDevice(), mCmdPool, 1, &mCmdBuffer);
    vkDestroyCommandPool(m_pDevice->GetDevice(), mCmdPool, nullptr);

    vkDestroyFence(m_pDevice->GetDevice(), mFence, nullptr);
}

UINT8 *UploadHeap::SubAllocate(SIZE_T uSize, UINT64 uAlign)
{
    // wait until we are done flusing the heap
    flushing.Wait();

    UINT8* pRet = nullptr;

    std::unique_lock<std::mutex> lock(mMutex);

    // make sure resource (and its mips) would fit the upload heap, if not please make the upload heap bigger
    assert(uSize < (size_t)(m_pDataBegin - m_pDataEnd));

    m_pDataCur = reinterpret_cast<UINT8*>(AlignUp(reinterpret_cast<SIZE_T>(m_pDataCur), uAlign));
    uSize = AlignUp(uSize, uAlign);

    // return nullptr if we ran out of space in the heap
    if ((m_pDataCur >= m_pDataEnd) || (m_pDataCur + uSize >= m_pDataEnd))
    {
        return nullptr;
    }

    pRet = m_pDataCur;
    m_pDataCur += uSize;

    return pRet;
}

UINT8 *UploadHeap::BeginSubAllocate(SIZE_T uSize, UINT64 uAlign)
{
    UINT8* pRes = nullptr;

    for (;;)
    {
        pRes = SubAllocate(uSize, uAlign);
        if (pRes != nullptr) break;

        FlushAndFinish();
    }
    allocating.Inc();

    return pRes;
}

void UploadHeap::EndSubAllocate()
{
    allocating.Dec();
}

void UploadHeap::AddCopy(VkImage image, VkBufferImageCopy bufferImageCopy)
{
    std::unique_lock<std::mutex> lock(mMutex);
    mCopies.push_back({ image, bufferImageCopy });
}

void UploadHeap::AddPreBarrier(VkImageMemoryBarrier imageMemoryBarrier)
{
    std::unique_lock<std::mutex> lock(mMutex);
    mToPreBarrier.push_back(imageMemoryBarrier);
}

void UploadHeap::AddPostBarrier(VkImageMemoryBarrier imageMemoryBarrier)
{
    std::unique_lock<std::mutex> lock(mMutex);
    mToPostBarrier.push_back(imageMemoryBarrier);
}

void UploadHeap::Flush()
{
    VkMappedMemoryRange range[1]{};
    range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range[0].memory = mDeviceMemory;
    range[0].size = m_pDataCur - m_pDataBegin;
    VK_CHECK_RESULT(vkFlushMappedMemoryRanges(m_pDevice->GetDevice(), 1, range));
}

void UploadHeap::FlushAndFinish(bool bDoBarriers)
{
    // make sure another thread is not already flushing
    flushing.Wait();

    // begins a critical section, and make sure no allocations happen while a thread is inside it
    flushing.Inc();

    // wait for pending allocations to finish
    allocating.Wait();

    std::unique_lock<std::mutex> lock(mMutex);
    Flush();
    Trace("flushing %i", mCopies.size());

    //apply pre barriers in one go
    if (!mToPreBarrier.empty())
    {
        vkCmdPipelineBarrier(
            GetCommandList(),
            VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0,nullptr, 0, nullptr,
            (uint32_t)mToPreBarrier.size(), mToPreBarrier.data());
        mToPreBarrier.clear();
    }

    for (COPY c : mCopies)
    {
        vkCmdCopyBufferToImage(
            GetCommandList(), GetResource(),
            c.mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &c.mBufferImageCopy);
    }
    mCopies.clear();

    //apply post barriers in one go
    if (!mToPostBarrier.empty())
    {
        vkCmdPipelineBarrier(
            GetCommandList(),
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr,
            (uint32_t)mToPostBarrier.size(), mToPostBarrier.data());
        mToPostBarrier.clear();
    }

    VK_CHECK_RESULT(vkEndCommandBuffer(mCmdBuffer));
    
    // Submit
    const VkCommandBuffer cmdBuffers[] = { mCmdBuffer };
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = cmdBuffers;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    VK_CHECK_RESULT(vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, mFence));

    // Make sure it's been processed by the GPU

    VK_CHECK_RESULT(vkWaitForFences(m_pDevice->GetDevice(), 1, &mFence, VK_TRUE, UINT64_MAX));

    vkResetFences(m_pDevice->GetDevice(), 1, &mFence);

    // Reset so it can be reused
    VkCommandBufferBeginInfo cmdBufferBI = {};
    cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK_RESULT(vkBeginCommandBuffer(mCmdBuffer, &cmdBufferBI))

    m_pDataCur = m_pDataBegin;

    flushing.Dec();
}
