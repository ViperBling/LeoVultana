#include "PCHVK.h"
#include "ResourceViewHeapsVK.h"
#include "HelperVK.h"
#include "Misc.h"

using namespace LeoVultana_VK;

void ResourceViewHeaps::OnCreate(
    Device *pDevice, uint32_t cbvDescriptorCount, uint32_t srvDescriptorCount,
    uint32_t uavDescriptorCount, uint32_t samplerDescriptorCount)
{
    m_pDevice = pDevice;
    mAllocateDescriptorCount = 0;

    const VkDescriptorPoolSize typeCount[] =
    {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, cbvDescriptorCount },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, cbvDescriptorCount },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, srvDescriptorCount },
        { VK_DESCRIPTOR_TYPE_SAMPLER, samplerDescriptorCount },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, uavDescriptorCount }
    };

    VkDescriptorPoolCreateInfo descPoolCI{};
    descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolCI.pNext = nullptr;
    descPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descPoolCI.maxSets = 8000;
    descPoolCI.poolSizeCount = _countof( typeCount );
    descPoolCI.pPoolSizes = typeCount;

    VK_CHECK_RESULT(vkCreateDescriptorPool(m_pDevice->GetDevice(), &descPoolCI, nullptr, &mDescriptorPool));
}

void ResourceViewHeaps::OnDestroy()
{
    vkDestroyDescriptorPool(m_pDevice->GetDevice(), mDescriptorPool, nullptr);
}

bool ResourceViewHeaps::AllocDescriptor(VkDescriptorSetLayout descriptorLayout, VkDescriptorSet *pDescriptorSet)
{
    std::lock_guard<std::mutex> lock(mMutex);

    VkDescriptorSetAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.descriptorPool = mDescriptorPool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptorLayout;

    VK_CHECK_RESULT(vkAllocateDescriptorSets(m_pDevice->GetDevice(), &alloc_info, pDescriptorSet));

    mAllocateDescriptorCount++;

    return true;
}

bool ResourceViewHeaps::AllocDescriptor(
    int size, const VkSampler *pSamplers,
    VkDescriptorSetLayout *pDescSetLayout,
    VkDescriptorSet *pDescriptorSet)
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings(size);
    for (int i = 0; i < size; i++)
    {
        layoutBindings[i].binding = i;
        layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[i].descriptorCount = 1;
        layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[i].pImmutableSamplers = (pSamplers != nullptr) ? &pSamplers[i] : nullptr;
    }

    return CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, pDescSetLayout, pDescriptorSet);
}

bool ResourceViewHeaps::AllocDescriptor(
    std::vector<uint32_t> &descriptorCounts, const VkSampler *pSamplers,
    VkDescriptorSetLayout *pDescSetLayout, VkDescriptorSet *pDescriptorSet)
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings(descriptorCounts.size());
    for (int i = 0; i < descriptorCounts.size(); i++)
    {
        layoutBindings[i].binding = i;
        layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[i].descriptorCount = descriptorCounts[i];
        layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[i].pImmutableSamplers = (pSamplers != nullptr) ? &pSamplers[i] : nullptr;
    }

    return CreateDescriptorSetLayoutAndAllocDescriptorSet(&layoutBindings, pDescSetLayout, pDescriptorSet);
}

bool ResourceViewHeaps::CreateDescriptorSetLayout(
    std::vector<VkDescriptorSetLayoutBinding> *pDescriptorLayoutBinding,
    VkDescriptorSetLayout *pDescSetLayout)
{
    // Next take layout bindings and use them to create a descriptor set layout

    VkDescriptorSetLayoutCreateInfo descSetLayoutCI = {};
    descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descSetLayoutCI.pNext = nullptr;
    descSetLayoutCI.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
    descSetLayoutCI.pBindings = pDescriptorLayoutBinding->data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_pDevice->GetDevice(), &descSetLayoutCI, nullptr, pDescSetLayout));

    return true;
}

bool ResourceViewHeaps::CreateDescriptorSetLayoutAndAllocDescriptorSet(
    std::vector<VkDescriptorSetLayoutBinding> *pDescriptorLayoutBinding,
    VkDescriptorSetLayout *pDescSetLayout,
    VkDescriptorSet *pDescriptorSet)
{
    // Next take layout bindings and use them to create a descriptor set layout

    VkDescriptorSetLayoutCreateInfo descSetLayoutCI = {};
    descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descSetLayoutCI.pNext = nullptr;
    descSetLayoutCI.bindingCount = (uint32_t)pDescriptorLayoutBinding->size();
    descSetLayoutCI.pBindings = pDescriptorLayoutBinding->data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_pDevice->GetDevice(), &descSetLayoutCI, nullptr, pDescSetLayout));

    return AllocDescriptor(*pDescSetLayout, pDescriptorSet);
}

void ResourceViewHeaps::FreeDescriptor(VkDescriptorSet descriptorSet)
{
    mAllocateDescriptorCount--;
    vkFreeDescriptorSets(m_pDevice->GetDevice(), mDescriptorPool, 1, &descriptorSet);
}
