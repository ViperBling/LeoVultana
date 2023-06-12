#pragma once

#include "RHI/Vulkan/VKCommon/StaticBufferPoolVK.h"
#include "RHI/Vulkan/VKCommon/DynamicBufferRingVK.h"
#include "RHI/Vulkan/VKCommon/ResourceViewHeapsVK.h"

namespace LeoVultana_VK
{
    class PostProcessPS
    {
    public:
        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            const std::string &shaderFilename,
            const std::string &shaderEntryPoint,
            const std::string &shaderCompilerParams,
            StaticBufferPool *pStaticBufferPool,
            DynamicBufferRing *pDynamicBufferRing,
            VkDescriptorSetLayout descriptorSetLayout,
            VkPipelineColorBlendStateCreateInfo *pBlendDesc = nullptr,
            VkSampleCountFlagBits sampleDescCount = VK_SAMPLE_COUNT_1_BIT
        );
        void OnDestroy();
        void UpdatePipeline(
            VkRenderPass renderPass,
            VkPipelineColorBlendStateCreateInfo *pBlendDesc = nullptr,
            VkSampleCountFlagBits sampleDescCount = VK_SAMPLE_COUNT_1_BIT);
        void Draw(VkCommandBuffer cmdBuffer, VkDescriptorBufferInfo *pConstantBuffer, VkDescriptorSet descriptorSet = VK_NULL_HANDLE);

    private:
        Device* m_pDevice;
        std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
        std::string mFragmentShaderName;

        uint32_t mNumIndices;
        VkIndexType mIndexType;
        VkDescriptorBufferInfo mIBV;

        VkPipeline mPipeline{};
        VkRenderPass mRenderPass{};
        VkPipelineLayout mPipelineLayout{};
    };
}