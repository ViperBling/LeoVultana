#include "PCHVK.h"
#include "UploadHeapVK.h"
#include "HelperVK.h"
#include "Misc.h"

using namespace LeoVultana_VK;

void UploadHeap::OnCreate(Device *pDevice, SIZE_T uSize)
{

}

void UploadHeap::OnDestroy()
{

}

UINT8 *UploadHeap::SubAllocate(SIZE_T uSize, UINT64 uAlign)
{
    return nullptr;
}

UINT8 *UploadHeap::BeginSubAllocate(SIZE_T uSize, UINT64 uAlign)
{
    return nullptr;
}

void UploadHeap::EndSubAllocate()
{

}

void UploadHeap::AddCopy(VkImage image, VkBufferImageCopy bufferImageCopy)
{

}

void UploadHeap::AddPreBarrier(VkImageMemoryBarrier imageMemoryBarrier)
{

}

void UploadHeap::AddPostBarrier(VkImageMemoryBarrier imageMemoryBarrier)
{

}

void UploadHeap::Flush()
{

}

void UploadHeap::FlushAndFinish(bool bDoBarriers)
{

}
