#pragma once

#include "DeviceVK.h"
#include "StaticBufferPoolVK.h"
#include "ResourceViewHeapsVK.h"
#include "DynamicBufferRingVK.h"
#include "GLTFTexturesAndBuffersVK.h"
#include "Widgets/WireframeBoxVK.h"

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