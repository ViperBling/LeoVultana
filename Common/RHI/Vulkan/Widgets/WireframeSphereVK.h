#pragma once

#include "WireframeVK.h"
#include "Function/WirePrimitives.h"

namespace LeoVultana_VK
{
    class WireframeSphere : public Wireframe
    {
    public:
        void OnCreate(
            Device* pDevice,
            ResourceViewHeaps *pResourceViewHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool)
        {
            std::vector<unsigned short> indices;
            std::vector<float> vertices;

            GenerateSphere(16, indices, vertices);

            // set indices
            mNumIndices = (uint32_t)indices.size();
            pStaticBufferPool->AllocBuffer(mNumIndices, sizeof(short), indices.data(), &mIBV);
            pStaticBufferPool->AllocBuffer((uint32_t)(vertices.size() / 3), (uint32_t)(3 * sizeof(float)), vertices.data(), &mVBV);
        }

        void OnDestroy() {}

        void Draw(
            VkCommandBuffer cmdBuffer,
            Wireframe *pWireframe,
            const math::Matrix4& worldMatrix,
            const math::Vector4& vCenter,
            const math::Vector4& vRadius,
            const math::Vector4& vColor)
        {
            pWireframe->Draw(cmdBuffer, mNumIndices, mIBV, mVBV, worldMatrix, vCenter, vRadius, vColor);
        }

    private:
        // all bounding boxes of all the meshes use the same geometry, shaders and pipelines.
        uint32_t                mNumIndices;
        VkDescriptorBufferInfo  mIBV;        // Index Buffer
        VkDescriptorBufferInfo  mVBV;        // Vertex Buffer
    };
}
