#include "CommandListRingVK.h"
#include "ExtDebugUtilsVK.h"
#include "HelperVK.h"
#include "Misc.h"

using namespace LeoVultana_VK;

void CommandListRing::OnCreate(Device *pDevice, uint32_t numOfBackBuffers, uint32_t cmdListPerFrame, bool compute)
{
    m_pDevice = pDevice;
    mNumOfAllocators = numOfBackBuffers;
    mCommandListPerBackBuffer = cmdListPerFrame;
    m_pCommandBuffers = new CommandBuffersPerFrame[mNumOfAllocators]();

    // 创建Command Allocators，每个Frame一个
    for (uint32_t i = 0; i < mNumOfAllocators; i++)
    {
        CommandBuffersPerFrame *pCmdBufferPerFrame = &m_pCommandBuffers[i];
        // 创建Command Pool
        VkCommandPoolCreateInfo cmdPoolCI{};
        cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolCI.pNext = nullptr;
        if (!compute)
        {
            cmdPoolCI.queueFamilyIndex = pDevice->GetGraphicsQueueFamilyIndex();
        } else
        {
            cmdPoolCI.queueFamilyIndex = pDevice->GetComputeQueueFamilyIndex();
        }
        cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        VK_CHECK_RESULT(vkCreateCommandPool(pDevice->GetDevice(), &cmdPoolCI, nullptr, &pCmdBufferPerFrame->mCmdPool));

        // 创建Command Buffer
        pCmdBufferPerFrame->m_pCmdBuffer = new VkCommandBuffer[mCommandListPerBackBuffer];
        VkCommandBufferAllocateInfo cmdBufferAI{};
        cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferAI.pNext = nullptr;
        cmdBufferAI.commandPool = pCmdBufferPerFrame->mCmdPool;
        cmdBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferAI.commandBufferCount = mCommandListPerBackBuffer;
        VK_CHECK_RESULT(vkAllocateCommandBuffers(pDevice->GetDevice(), &cmdBufferAI, pCmdBufferPerFrame->m_pCmdBuffer));
        pCmdBufferPerFrame->mUsedCmdList = 0;
    }
    mFrameIndex = 0;
    m_pCurrentFrame = &m_pCommandBuffers[mFrameIndex % mNumOfAllocators];
    mFrameIndex++;
    m_pCurrentFrame->mUsedCmdList = 0;
}

void CommandListRing::OnDestroy()
{
    for (uint32_t i = 0; i < mNumOfAllocators; i++)
    {
        vkFreeCommandBuffers(m_pDevice->GetDevice(), m_pCommandBuffers[i].mCmdPool, mCommandListPerBackBuffer, m_pCommandBuffers[i].m_pCmdBuffer);
        vkDestroyCommandPool(m_pDevice->GetDevice(), m_pCommandBuffers[i].mCmdPool, nullptr);
    }
    delete[] m_pCommandBuffers;
}

void CommandListRing::OnBeginFrame()
{
    m_pCurrentFrame = &m_pCommandBuffers[mFrameIndex % mNumOfAllocators];
    m_pCurrentFrame->mUsedCmdList = 0;
    mFrameIndex++;
}

VkCommandBuffer CommandListRing::GetNewCommandList()
{
    VkCommandBuffer cmdBuffer = m_pCurrentFrame->m_pCmdBuffer[m_pCurrentFrame->mUsedCmdList++];
    assert(m_pCurrentFrame->mUsedCmdList < mCommandListPerBackBuffer);
    return cmdBuffer;
}
