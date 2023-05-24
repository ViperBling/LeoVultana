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

#ifdef USE_VMA
    VkBufferCreateInfo bufferCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferCI.size = mMemTotalSize;
    bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo vmaAllocationCI{};
    vmaAllocationCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaAllocationCI.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    vmaAllocationCI.pUserData = name;

    VK_CHECK_RESULT(vmaCreateBuffer(
        pDevice->GetAllocator(), &bufferCI,
        &vmaAllocationCI, &mBuffer, &mBufferAllocation,nullptr));
    SetResourceName(pDevice->GetDevice(), VK_OBJECT_TYPE_BUFFER, (uint64_t)mBuffer, "DynamicBufferRing");
    VK_CHECK_RESULT(vmaMapMemory(pDevice->GetAllocator(), mBufferAllocation, (void**)&m_pData));
#else
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

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(m_pDevice->GetDevice(), mBuffer, &memRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.memoryTypeIndex = 0;
    allocateInfo.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = 0;

    auto deviceMemProp = m_pDevice->GetPhysicalDeviceMemoryProperties();
    bool pass = MemoryTypeFromProperties(
        deviceMemProp,
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocateInfo.memoryTypeIndex);
    assert(pass && "No mappable, coherent memory.");

    VK_CHECK_RESULT(vkAllocateMemory(m_pDevice->GetDevice(), &allocateInfo, nullptr, &mDeviceMemory))
    VK_CHECK_RESULT(vkMapMemory(m_pDevice->GetDevice(), mDeviceMemory, 0, memRequirements.size, 0, (void **)&m_pData));
    VK_CHECK_RESULT(vkBindBufferMemory(m_pDevice->GetDevice(), mBuffer, mDeviceMemory, 0));
#endif
}

void DynamicBufferRing::OnDestroy()
{
#ifdef USE_VMA
    vmaUnmapMemory(m_pDevice->GetAllocator(), mBufferAllocation);
    vmaDestroyBuffer(m_pDevice->GetAllocator(), mBuffer, mBufferAllocation);
#else
    vkUnmapMemory(m_pDevice->GetDevice(), mDeviceMemory);
    vkFreeMemory(m_pDevice->GetDevice(), mDeviceMemory, nullptr);
    vkDestroyBuffer(m_pDevice->GetDevice(), mBuffer, nullptr);
#endif
    mMem.OnDestroy();
}

bool DynamicBufferRing::AllocateConstantBuffer(uint32_t size, void **pData, VkDescriptorBufferInfo *pOut)
{
    size = AlignUp(size, 256u);

    uint32_t memOffset;
    if (!mMem.Alloc(size, &memOffset))
    {
        assert("Ran out of memory for 'dynamic' buffers, please increase the allocated size");
        return false;
    }
    *pData = (void*)(m_pData + memOffset);

    pOut->buffer = mBuffer;
    pOut->offset = memOffset;
    pOut->range = size;

    return true;
}

VkDescriptorBufferInfo DynamicBufferRing::AllocateConstantBuffer(uint32_t size, void *pData)
{
    void * pBuffer;
    VkDescriptorBufferInfo out;
    if (AllocateConstantBuffer(size, &pBuffer, &out)) memcpy(pBuffer, pData, size);
    return out;
}

bool DynamicBufferRing::AllocateVertexBuffer(
    uint32_t numberOfVertices, uint32_t strideInBytes,
    void **pData, VkDescriptorBufferInfo *pOut)
{
    return AllocateConstantBuffer(numberOfVertices * strideInBytes, pData, pOut);
}

bool DynamicBufferRing::AllocateIndexBuffer(
    uint32_t numberOfIndices, uint32_t strideInBytes,
    void **pData, VkDescriptorBufferInfo *pOut)
{
    return AllocateConstantBuffer(numberOfIndices * strideInBytes, pData, pOut);
}

void DynamicBufferRing::OnBeginFrame()
{
    mMem.OnBeginFrame();
}

void DynamicBufferRing::SetDescriptorSet(int index, uint32_t size, VkDescriptorSet descriptorSet)
{
    VkDescriptorBufferInfo out{};
    out.buffer = mBuffer;
    out.offset = 0;
    out.range = size;

    VkWriteDescriptorSet write;
    write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.dstSet = descriptorSet;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write.pBufferInfo = &out;
    write.dstArrayElement = 0;
    write.dstBinding = index;

    vkUpdateDescriptorSets(m_pDevice->GetDevice(), 1, &write, 0, nullptr);
}
