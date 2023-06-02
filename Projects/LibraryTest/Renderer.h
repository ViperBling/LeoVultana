#pragma once

#include "ProjectPCH.h"
#include "GLTF/GLTFCommon.h"
#include "Function/Async.h"
#include "RHI/Vulkan/PostProcess/MagnifierPS.h"
#include "Utilities/Benchmark.h"

// We are queuing (backBufferCount + 0.5) frames, so we need to triple buffer the resources that get modified each frame
static const int backBufferCount = 3;

using namespace LeoVultana_VK;

class UIState;

// Renderer class is responsible for rendering resources management and recording command buffers.
class Renderer
{
public:
    void OnCreate(Device *pDevice, SwapChain *pSwapChain, float FontSize);
    void OnDestroy();

    void OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height);
    void OnDestroyWindowSizeDependentResources();

    void OnUpdateDisplayDependentResources(SwapChain *pSwapChain);

    int LoadScene(GLTFCommon *pGLTFCommon, int Stage = 0);
    void UnloadScene();

    void AllocateShadowMaps(GLTFCommon* pGLTFCommon);

    const std::vector<TimeStamp> &GetTimingValues() { return mTimeStamps; }

    void OnRender(const UIState* pState, const Camera& camera, SwapChain* pSwapChain);

private:
    Device*                         m_pDevice;

    uint32_t                        mWidth;
    uint32_t                        mHeight;

    VkRect2D                        mRectScissor;
    VkViewport                      mViewport;

    // Initialize helper classes
    ResourceViewHeaps               mResourceViewHeaps;
    UploadHeap                      mUploadHeap;
    DynamicBufferRing               mConstantBufferRing;
    StaticBufferPool                mVidMemBufferPool;
    StaticBufferPool                mSysMemBufferPool;
    CommandListRing                 mCommandListRing;
    GPUTimeStamps                   mGPUTimer;

    //gltf passes
    GLTFBaseMeshPass*               m_pGLTFBasePass;
    GLTFDepthPass*                  m_pGLTFDepthPass;
    GLTFTexturesAndBuffers*         m_pGLTFTexturesAndBuffers;

    // GUI
    ImGUI                           mImGUI;

    // GBuffer and render passes
    GBuffer                         mGBuffer;
    GBufferRenderPass               mRenderPassFullGBufferWithClear;
    GBufferRenderPass               mRenderPassJustDepthAndHDR;
    GBufferRenderPass               mRenderPassFullGBuffer;

    // shadowmaps
    VkRenderPass                    mRenderPassShadow;

    typedef struct {
        Texture         ShadowMap;
        uint32_t        ShadowIndex;
        uint32_t        ShadowResolution;
        uint32_t        LightIndex;
        VkImageView     ShadowDSV;
        VkFramebuffer   ShadowFrameBuffer;
    } SceneShadowInfo;

    std::vector<SceneShadowInfo>    mShadowMapPool;
    std::vector< VkImageView>       mShadowSRVPool;

    std::vector<TimeStamp>          mTimeStamps;

    AsyncPool                       mAsyncPool;
};
