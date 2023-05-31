#pragma once

#include "DynamicBufferRingVK.h"
#include "CommandListRingVK.h"
#include "UploadHeapVK.h"
#include "ImGui/imgui.h"
#include "Utilities/ImGuiHelper.h"

namespace LeoVultana_VK
{
    class ImGUI
    {
    public:
        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            UploadHeap* pUploadHeap,
            DynamicBufferRing *pConstantBufferRing,
            float fontSize = 13.f);
        void OnDestroy();

        void UpdatePipeline(VkRenderPass renderPass);
        void Draw(VkCommandBuffer cmdBuffer);

    private:
        Device*                     m_pDevice = nullptr;
        DynamicBufferRing*          m_pConstBuf = nullptr;

        VkImage                     mTexture2D{};
        VmaAllocation               mImageAlloc{};
        VkDescriptorBufferInfo      mGeometry{};
        VkPipelineLayout            mPipelineLayout{};
        VkDescriptorPool            mDescriptorPool{};
        VkPipeline                  mPipeline{};
        VkDescriptorSet             mDescriptorSet[128];
        uint32_t                    mCurrentDescriptorIndex;
        VkSampler                   mSampler{};
        VkImageView                 mTextureSRV{};
        VkDescriptorSetLayout       mDescLayout{};
        std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
    };
}