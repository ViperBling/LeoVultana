#include "PCHVK.h"
#include "DynamicBufferRingVK.h"
#include "Misc.h"
#include "ExtDebugUtilsVK.h"
#include "HelperVK.h"

using namespace LeoVultana_VK;

void DynamicBufferRing::OnCreate(Device *pDevice, uint32_t numberOfBackBuffers, uint32_t memTotalSize, char *name)
{
    m_pDevice = pDevice;
    mMemTotalSize = memTotalSize;
    mMem.OnCreate(numberOfBackBuffers, mMemTotalSize);

//#ifdef USE_VMA
//    VkBufferCreateInfo bufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
//    bufferCI.size = mMemTotalSize;
//    bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
//
//    VmaAllocationCreateInfo vmaAllocationCI{};
//    vmaAllocationCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
//    vmaAllocationCI.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
//    vmaAllocationCI.pUserData = name;
//
//    VK_CHECK_RESULT(vmaCreateBuffer(
//        pDevice->GetAllocator(), &bufferCI,
//        &vmaAllocationCI, &mBuffer, &mBufferAllocation,nullptr));
//    SetResourceName(pDevice->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)mBuffer, "DynamicBufferRing");
//    VK_CHECK_RESULT(vmaMapMemory(pDevice->GetAllocator(), mBufferAllocation, (void**)&m_pData));
//#else
    // 创建一个可以保存uniform、顶点、索引的Buffer
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.pNext = nullptr;
    bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |  VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCI.size = mMemTotalSize;
    bufferCI.queueFamilyIndexCount = 0;
    bufferCI.pQueueFamilyIndices = nullptr;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCI.flags = 0;
    VK_CHECK_RESULT(vkCreateBuffer(m_pDevice->GetDevice(), &bufferCI, nullptr, &mBuffer));
//#endif
}

void DynamicBufferRing::OnDestroy()
{

}

bool DynamicBufferRing::AllocateConstantBuffer(uint32_t size, void **pData, VkDescriptorBufferInfo *pOut)
{
    return false;
}

VkDescriptorBufferInfo DynamicBufferRing::AllocateConstantBuffer(uint32_t size, void *pData)
{
    return VkDescriptorBufferInfo();
}

bool DynamicBufferRing::AllocateVertexBuffer(
    uint32_t numberOfVertices, uint32_t strideInBytes,
    void **pData, VkDescriptorBufferInfo *pOut)
{
    return false;
}

bool DynamicBufferRing::AllocateIndexBuffer(
    uint32_t numberOfIndices, uint32_t strideInBytes,
    void **pData, VkDescriptorBufferInfo *pOut)
{
    return false;
}

void DynamicBufferRing::OnBeginFrame()
{

}

void DynamicBufferRing::SetDescriptorSet(int i, uint32_t sizse, VkDescriptorSet descriptorSet)
{

}
