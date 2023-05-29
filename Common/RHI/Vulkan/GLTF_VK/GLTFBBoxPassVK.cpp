#include "PCHVK.h"
#include "GLTFBBoxPassVK.h"
#include "ExtDebugUtilsVK.h"
#include "GLTFHelpersVK.h"
#include "GLTFStructures.h"
#include "ShaderCompilerHelperVK.h"

using namespace LeoVultana_VK;

void GLTFBoxPass::OnCreate(
    Device *pDevice,
    VkRenderPass renderPass,
    ResourceViewHeaps *pHeaps,
    DynamicBufferRing *pDynamicBufferRing,
    StaticBufferPool *pStaticBufferPool,
    GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
    Wireframe *pWireframe)
{
    m_pWireframe = pWireframe;
    m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;

    mWireframeBox.OnCreate(pDevice, pHeaps, pDynamicBufferRing, pStaticBufferPool);
}

void GLTFBoxPass::OnDestroy()
{
    mWireframeBox.OnDestroy();
}

void
GLTFBoxPass::Draw(
    VkCommandBuffer cmdBuffer,
    const math::Matrix4 &cameraViewProjMatrix,
    const math::Vector4 &color)
{
    SetPerfMarkerBegin(cmdBuffer, "bounding boxes");

    GLTFCommon* pGLTFCommon = m_pGLTFTexturesAndBuffers->m_pGLTFCommon;

    for (uint32_t i = 0; i < pGLTFCommon->mNodes.size(); ++i)
    {
        gltfNode* pNode = &pGLTFCommon->mNodes[i];
        if (pNode->meshIndex < 0) continue;

        math::Matrix4 mWorldViewProj = cameraViewProjMatrix * pGLTFCommon->mWorldSpaceMats[i].GetCurrent();

        gltfMesh* pMesh = &pGLTFCommon->mMeshes[pNode->meshIndex];
        for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); ++p)
        {
            mWireframeBox.Draw(
                cmdBuffer, m_pWireframe,
                mWorldViewProj,
                pMesh->m_pPrimitives[p].mCenter,
                pMesh->m_pPrimitives[p].mRadius, color);
        }
    }
    SetPerfMarkerEnd(cmdBuffer);
}
