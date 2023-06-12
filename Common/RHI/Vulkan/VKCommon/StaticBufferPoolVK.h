#pragma once

#include "DeviceVK.h"
#include "ResourceViewHeapsVK.h"
#include "Vulkan/vk_mem_alloc.h"

namespace LeoVultana_VK
{
    // Simulates DX11 style static buffers. For dynamic buffers please see 'DynamicBufferRingDX12.h'
    //
    // This class allows suballocating small chuncks of memory from a huge buffer that is allocated on creation
    // This class is specialized in vertex and index buffers.
    class StaticBufferPool
    {
    public:
        void OnCreate(Device *pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name);
        void OnDestroy();

        // Allocates a IB/VB and returns a pointer to fill it + a descriptot
        bool AllocateBuffer(uint32_t numberOfVertices, uint32_t strideInBytes, void **pData, VkDescriptorBufferInfo *pOut);

        // Allocates a IB/VB and fill it with pInitData, returns a descriptor
        bool AllocateBuffer(uint32_t numberOfIndices, uint32_t strideInBytes, const void *pInitData, VkDescriptorBufferInfo *pOut);

        // if using vidmem this kicks the upload from the upload heap to the video mem
        void UploadData(VkCommandBuffer cmdBuffer);

        // if using vidmem frees the upload heap
        void FreeUploadHeap();

    private:
        Device*         m_pDevice;
        std::mutex      mMutex{};
        bool            m_bUseVidMem = true;            // 使用显存
        char*           m_pData = nullptr;
        uint32_t        mMemOffset = 0;
        uint32_t        mTotalMemSize = 0;

        VkBuffer        mBuffer;
        VkBuffer        mBufferVid;

#ifdef USE_VMA
        VmaAllocation   mBufferAlloc{};
        VmaAllocation   mBufferAllocVid{};
#else
        VkDeviceMemory  mDeviceMemory{};
        VkDeviceMemory  mDeviceMemoryVid{};
#endif
    };
}