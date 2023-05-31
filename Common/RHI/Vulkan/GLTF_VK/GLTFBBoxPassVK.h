#pragma once

#include "RHI/Vulkan/VKCommon/DeviceVK.h"
#include "RHI/Vulkan/VKCommon/StaticBufferPoolVK.h"
#include "RHI/Vulkan/VKCommon/ResourceViewHeapsVK.h"
#include "RHI/Vulkan/VKCommon/DynamicBufferRingVK.h"
#include "GLTFTexturesAndBuffersVK.h"
#include "RHI/Vulkan/Widgets/WireframeBoxVK.h"

namespace LeoVultana_VK
{
    class GLTFBoxPass
    {
    public:
        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
            Wireframe *pWireframe);

        void OnDestroy();
        void Draw(VkCommandBuffer cmdBuffer, const math::Matrix4& cameraViewProjMatrix, const math::Vector4& color);
        inline void Draw(VkCommandBuffer cmdBuffer, const math::Matrix4& cameraViewProjMatrix)
        {
            Draw(cmdBuffer, cameraViewProjMatrix, math::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        }

    private:
        GLTFTexturesAndBuffers* m_pGLTFTexturesAndBuffers;

        Wireframe*              m_pWireframe;
        WireframeBox            mWireframeBox;
    };
}