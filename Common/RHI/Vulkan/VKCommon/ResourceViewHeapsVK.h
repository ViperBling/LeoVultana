#pragma once

#include "DeviceVK.h"

namespace LeoVultana_VK
{
    // This class will create a Descriptor Pool and allows allocating, freeing and initializing Descriptor Set Layouts(DSL) from this pool.
    class ResourceViewHeaps
    {
    public:
        void OnCreate(
            Device *pDevice,
            uint32_t cbvDescriptorCount,
            uint32_t srvDescriptorCount,
            uint32_t uavDescriptorCount,
            uint32_t samplerDescriptorCount);
        void OnDestroy();

        bool AllocateDescriptor(
            VkDescriptorSetLayout descriptorLayout,
            VkDescriptorSet *pDescriptor);

        bool AllocateDescriptor(
            int size,
            const VkSampler *pSamplers,
            VkDescriptorSetLayout *pDescSetLayout,
            VkDescriptorSet *pDescriptor);

        bool AllocateDescriptor(
            std::vector<uint32_t> &descriptorCounts,
            const VkSampler* pSamplers,
            VkDescriptorSetLayout* pDescSetLayout,
            VkDescriptorSet* pDescriptor);

        bool CreateDescriptorSetLayout(
            std::vector<VkDescriptorSetLayoutBinding> *pDescriptorLayoutBinding,
            VkDescriptorSetLayout *pDescSetLayout);
        bool CreateDescriptorSetLayoutAndAllocDescriptorSet(
            std::vector<VkDescriptorSetLayoutBinding> *pDescriptorLayoutBinding,
            VkDescriptorSetLayout *pDescSetLayout,
            VkDescriptorSet *pDescriptor);
        void FreeDescriptor(VkDescriptorSet descriptorSet);

    private:
        Device*             m_pDevice;
        VkDescriptorPool    mDescriptorPool;
        std::mutex          mMutex;
        int                 mAllocateDescriptorCount{};
    };
}