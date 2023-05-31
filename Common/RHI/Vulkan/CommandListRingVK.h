#pragma once

#include "PCHVK.h"
#include "Function/Ring.h"
#include "DeviceVK.h"

namespace LeoVultana_VK
{
    // 这个类在创建的时候会使用Ring Buffer分配一些Command List。这样可以复用
    class CommandListRing
    {
    public:
        void OnCreate(Device* pDevice, uint32_t numOfBackBuffers, uint32_t cmdListPerFrame, bool compute = false);
        void OnDestroy();
        void OnBeginFrame();
        VkCommandBuffer GetNewCommandList();
        VkCommandPool GetPool() { return m_pCommandBuffers->mCmdPool; }


    private:
        uint32_t mFrameIndex;
        uint32_t mNumOfAllocators;
        uint32_t mCommandListPerBackBuffer;

        Device* m_pDevice;

        // m_pCommandBuffers保存每帧的所有CommandBuffer
        struct CommandBuffersPerFrame
        {
            VkCommandPool mCmdPool;
            VkCommandBuffer *m_pCmdBuffer;
            uint32_t mUsedCmdList;
        } *m_pCommandBuffers, *m_pCurrentFrame;
    };
}