#pragma once

#include "DeviceVK.h"
#include "Function/Async.h"

namespace LeoVultana_VK
{
    /*
     * This class shows the most efficient way to upload resources to the GPU memory.
     * The idea is to create just one upload heap and suballocate memory from it.
     * For convenience this class comes with its own command list & submit (FlushAndFinish)
     */
    class UploadHeap
    {
    public:
        void OnCreate(Device* pDevice, SIZE_T uSize);
        void OnDestroy();

        UINT8* SubAllocate(SIZE_T uSize, UINT64 uAlign);
        UINT8* BeginSubAllocate(SIZE_T uSize, UINT64 uAlign);
        void EndSubAllocate();

        UINT8* BasePtr() { return m_pDataBegin; }
        VkBuffer GetResource() { return mBuffer; }
        VkCommandBuffer GetCommandList() { return mCmdBuffer; }

        void AddCopy(VkImage image, VkBufferImageCopy bufferImageCopy);
        void AddPreBarrier(VkImageMemoryBarrier imageMemoryBarrier);
        void AddPostBarrier(VkImageMemoryBarrier imageMemoryBarrier);

        void Flush();
        void FlushAndFinish(bool bDoBarriers=false);

    private:
        Sync allocating, flushing;
        struct COPY
        {
            VkImage mImage;
            VkBufferImageCopy mBufferImageCopy;
        };
        std::vector<COPY> mCopies;
        std::vector<VkImageMemoryBarrier> mToPreBarrier;
        std::vector<VkImageMemoryBarrier> mToPostBarrier;

        std::mutex mMutex;

    private:
        Device* m_pDevice;
        VkCommandPool mCmdPool;
        VkCommandBuffer mCmdBuffer;

        VkBuffer mBuffer;
        VkDeviceMemory mDeviceMemory;

        VkFence mFence;

        UINT8* m_pDataBegin = nullptr;  // starting position of upload heap
        UINT8* m_pDataCur   = nullptr;    // current position of upload heap
        UINT8* m_pDataEnd   = nullptr;    // ending position of upload heap
    };
}