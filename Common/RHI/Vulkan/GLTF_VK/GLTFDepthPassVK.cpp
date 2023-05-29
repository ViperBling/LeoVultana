#include "PCHVK.h"
#include "GLTFDepthPassVK.h"
#include "GLTFHelpersVK.h"
#include "ShaderCompilerHelperVK.h"
#include "ExtDebugUtilsVK.h"
#include "ResourceViewHeapsVK.h"
#include "HelperVK.h"
#include "Async.h"
#include "GLTFPBRMaterial.h"

using namespace LeoVultana_VK;

void GLTFDepthPass::OnCreate(
    Device *pDevice,
    VkRenderPass renderPass,
    UploadHeap *pUploadHeap,
    ResourceViewHeaps *pHeaps,
    DynamicBufferRing *pDynamicBufferRing,
    StaticBufferPool *pStaticBufferPool,
    GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
    AsyncPool *pAsyncPool)
{

}

void GLTFDepthPass::OnDestroy()
{

}

GLTFDepthPass::PerFrame *GLTFDepthPass::SetPerFrameConstants()
{
    return nullptr;
}

void GLTFDepthPass::Draw(VkCommandBuffer cmdBuffer)
{

}

void GLTFDepthPass::CreateDescriptors(
    int inverseMatrixBufferSize,
    DefineList *pAttributeDefines,
    DepthPrimitives *pPrimitives)
{

}

void GLTFDepthPass::CreatePipeline(
    std::vector<VkVertexInputAttributeDescription> layout,
    const DefineList &defineList,
    DepthPrimitives *pPrimitives)
{

}
