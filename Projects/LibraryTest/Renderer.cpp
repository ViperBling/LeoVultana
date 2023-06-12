#include "Renderer.h"
#include "UIState.h"

void Renderer::OnCreate(Device *pDevice, SwapChain *pSwapChain, float FontSize)
{
    m_pDevice = pDevice;

    const uint32_t cbvDescCount = 2000;
    const uint32_t srvDescCount = 8000;
    const uint32_t uavDescCount = 10;
    const uint32_t samplerDescCount = 20;
    mResourceViewHeaps.OnCreate(m_pDevice, cbvDescCount, srvDescCount, uavDescCount, samplerDescCount);

    uint32_t cmdListPerBackBuffer = 8;
    mCommandListRing.OnCreate(m_pDevice, backBufferCount, cmdListPerBackBuffer);

    const uint32_t constantBufferMemSize = 100 * 1024 * 1024;
    mConstantBufferRing.OnCreate(m_pDevice, backBufferCount, constantBufferMemSize, "Uniforms");

    const uint32_t staticGeometryMemSize = 64 * 1024 * 1024;
    mVidMemBufferPool.OnCreate(m_pDevice, staticGeometryMemSize, true, "StaticGeom");

    const uint32_t systemGeometryMemSize = 32 * 1024;
    mSysMemBufferPool.OnCreate(m_pDevice, systemGeometryMemSize, false, "PostProcGeom");

    mGPUTimer.OnCreate(m_pDevice, backBufferCount);

    const uint32_t uploadHeapMemSize = 1000 * 1024 * 1024;
    mUploadHeap.OnCreate(m_pDevice, uploadHeapMemSize);

    // GBuffer
    mGBuffer.OnCreate(
        m_pDevice, &mResourceViewHeaps,
        {
            { GBUFFER_DEPTH, VK_FORMAT_D32_SFLOAT},
            { GBUFFER_FORWARD, VK_FORMAT_R16G16B16A16_SFLOAT},
        },
        1
        );
    GBufferFlags  fullGBuffer = GBUFFER_DEPTH | GBUFFER_FORWARD;
    bool bClear = true;
    mRenderPassFullGBufferWithClear.OnCreate(&mGBuffer, fullGBuffer, bClear,"mRenderPassFullGBufferWithClear");
    mRenderPassFullGBuffer.OnCreate(&mGBuffer, fullGBuffer, !bClear, "mRenderPassFullGBuffer");
    mRenderPassJustDepthAndHDR.OnCreate(&mGBuffer, GBUFFER_DEPTH | GBUFFER_FORWARD, !bClear, "mRenderPassJustDepthAndHdr");

    VkAttachmentDescription depthAttachDesc{};
    AttachClearBeforeUse(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &depthAttachDesc);
    mRenderPassShadow = CreateRenderPassOptimal(m_pDevice->GetDevice(), 0, nullptr, &depthAttachDesc);

    mImGUI.OnCreate(m_pDevice, pSwapChain->GetRenderPass(), &mUploadHeap, &mConstantBufferRing, FontSize);
    mVidMemBufferPool.UploadData(mUploadHeap.GetCommandList());
    mUploadHeap.FlushAndFinish();
}

void Renderer::OnDestroy()
{
    mAsyncPool.Flush();

    mImGUI.OnDestroy();

    mRenderPassFullGBufferWithClear.OnDestroy();
    mRenderPassJustDepthAndHDR.OnDestroy();
    mRenderPassFullGBuffer.OnDestroy();

    vkDestroyRenderPass(m_pDevice->GetDevice(), mRenderPassShadow, nullptr);

    mUploadHeap.OnDestroy();
    mGPUTimer.OnDestroy();
    mVidMemBufferPool.OnDestroy();
    mSysMemBufferPool.OnDestroy();
    mConstantBufferRing.OnDestroy();
    mResourceViewHeaps.OnDestroy();
    mCommandListRing.OnDestroy();
}

void Renderer::OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height)
{
    mWidth = Width;
    mHeight = Height;

    // Set the viewport
    mViewport.x = 0;
    mViewport.y = (float)Height;
    mViewport.width = (float)Width;
    mViewport.height = -(float)(Height);
    mViewport.minDepth = (float)0.0f;
    mViewport.maxDepth = (float)1.0f;

    // Create scissor rectangle
    mRectScissor.extent.width = Width;
    mRectScissor.extent.height = Height;
    mRectScissor.offset.x = 0;
    mRectScissor.offset.y = 0;

    // Create GBuffer
    mGBuffer.OnCreateWindowSizeDependentResources(pSwapChain, Width, Height);

    // Create frame buffers for the GBuffer render passes
    mRenderPassFullGBufferWithClear.OnCreateWindowSizeDependentResources(Width, Height);
    mRenderPassJustDepthAndHDR.OnCreateWindowSizeDependentResources(Width, Height);
    mRenderPassFullGBuffer.OnCreateWindowSizeDependentResources(Width, Height);
}

void Renderer::OnDestroyWindowSizeDependentResources()
{
    mRenderPassFullGBufferWithClear.OnDestroyWindowSizeDependentResources();
    mRenderPassJustDepthAndHDR.OnDestroyWindowSizeDependentResources();
    mRenderPassFullGBuffer.OnDestroyWindowSizeDependentResources();
    mGBuffer.OnDestroyWindowSizeDependentResources();
}

void Renderer::OnUpdateDisplayDependentResources(SwapChain *pSwapChain)
{
    mImGUI.UpdatePipeline((pSwapChain->GetDisplayMode() == DISPLAYMODE_SDR) ?
        pSwapChain->GetRenderPass() : mRenderPassJustDepthAndHDR.GetRenderPass());
}

int Renderer::LoadScene(GLTFCommon *pGLTFCommon, int Stage)
{
    ImGui::OpenPopup("Loading");
    if (ImGui::BeginPopupModal("Loading", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        float progress = (float) Stage / 10.0f;
        ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), nullptr);
        ImGui::EndPopup();
    }

    AsyncPool* pAsyncPool = &mAsyncPool;

    if (Stage == 0){}
    else if (Stage == 6)
    {
        Profile p("m_pGLTFLoader->Load");
        m_pGLTFTexturesAndBuffers = new GLTFTexturesAndBuffers();
        m_pGLTFTexturesAndBuffers->OnCreate(
            m_pDevice, pGLTFCommon,
            &mUploadHeap,
            &mVidMemBufferPool, &mConstantBufferRing);
    }
    else if (Stage == 7)
    {
        Profile p("LoadTextures");
        m_pGLTFTexturesAndBuffers->LoadTextures(nullptr);
    }
    else if (Stage == 8)
    {
        Profile p("m_pGLTFDepthPass->OnCreate");
        m_pGLTFDepthPass = new GLTFDepthPass();
        m_pGLTFDepthPass->OnCreate(
            m_pDevice, mRenderPassShadow,
            &mUploadHeap, &mResourceViewHeaps,
            &mConstantBufferRing, &mVidMemBufferPool,
            m_pGLTFTexturesAndBuffers,
            nullptr
            );
        mVidMemBufferPool.UploadData(mUploadHeap.GetCommandList());
        mUploadHeap.FlushAndFinish();
    }
    else if (Stage == 9)
    {
        Profile p("m_pGLTFBasePass->OnCreate");
        m_pGLTFBasePass = new GLTFBaseMeshPass();
        m_pGLTFBasePass->OnCreate(
            m_pDevice,
            &mUploadHeap,
            &mResourceViewHeaps,
            &mConstantBufferRing,
            &mVidMemBufferPool,
            m_pGLTFTexturesAndBuffers,
            mShadowSRVPool,
            &mRenderPassFullGBufferWithClear,
            nullptr
            );
        mVidMemBufferPool.UploadData(mUploadHeap.GetCommandList());
        mUploadHeap.FlushAndFinish();
    }
    else if (Stage == 10)
    {
        Profile p("Flush");

        mUploadHeap.Flush();
        mVidMemBufferPool.FreeUploadHeap();

        return 0;
    }
    Stage++;
    return Stage;
}

void Renderer::UnloadScene()
{
    // wait for all the async loading operations to finish
    mAsyncPool.Flush();

    m_pDevice->GPUFlush();

    if (m_pGLTFBasePass)
    {
        m_pGLTFBasePass->OnDestroy();
        delete m_pGLTFBasePass;
        m_pGLTFBasePass = nullptr;
    }
    if (m_pGLTFDepthPass)
    {
        m_pGLTFDepthPass->OnDestroy();
        delete m_pGLTFDepthPass;
        m_pGLTFDepthPass = nullptr;
    }
    if (m_pGLTFTexturesAndBuffers)
    {
        m_pGLTFTexturesAndBuffers->OnDestroy();
        delete m_pGLTFTexturesAndBuffers;
        m_pGLTFTexturesAndBuffers = nullptr;
    }
    assert(mShadowMapPool.size() == mShadowSRVPool.size());
    while(!mShadowMapPool.empty())
    {
        mShadowMapPool.back().ShadowMap.OnDestroy();
        vkDestroyFramebuffer(m_pDevice->GetDevice(), mShadowMapPool.back().ShadowFrameBuffer, nullptr);
        vkDestroyImageView(m_pDevice->GetDevice(), mShadowSRVPool.back(), nullptr);
        vkDestroyImageView(m_pDevice->GetDevice(), mShadowMapPool.back().ShadowDSV, nullptr);
        mShadowSRVPool.pop_back();
        mShadowMapPool.pop_back();
    }
}

void Renderer::AllocateShadowMaps(GLTFCommon *pGLTFCommon)
{
    // Go through the lights and allocate shadow information
    uint32_t NumShadows = 0;
    for (int i = 0; i < pGLTFCommon->mLightInstances.size(); ++i)
    {
        const gltfLight& lightData = pGLTFCommon->mLights[pGLTFCommon->mLightInstances[i].mLightId];
        if (lightData.mShadowResolution)
        {
            SceneShadowInfo ShadowInfo;
            ShadowInfo.ShadowResolution = lightData.mShadowResolution;
            ShadowInfo.ShadowIndex = NumShadows++;
            ShadowInfo.LightIndex = i;
            mShadowMapPool.push_back(ShadowInfo);
        }
    }

    if (NumShadows > MaxShadowInstances)
    {
        Trace("Number of shadows has exceeded maximum supported. Please grow value in gltfCommon.h/perFrameStruct.h");
        throw;
    }

    // If we had shadow information, allocate all required maps and bindings
    if (!mShadowMapPool.empty())
    {
        auto CurrentShadow = mShadowMapPool.begin();
        for (uint32_t i = 0; CurrentShadow < mShadowMapPool.end(); ++i, ++CurrentShadow)
        {
            CurrentShadow->ShadowMap.InitDepthStencil(m_pDevice, CurrentShadow->ShadowResolution, CurrentShadow->ShadowResolution, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, "ShadowMap");
            CurrentShadow->ShadowMap.CreateDSV(&CurrentShadow->ShadowDSV);

            // Create render pass shadow, will clear contents

            VkAttachmentDescription depthAttachments;
            AttachClearBeforeUse(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &depthAttachments);

            // Create frame buffer
            VkImageView attachmentViews[1] = { CurrentShadow->ShadowDSV };
            VkFramebufferCreateInfo shadowFrameBufferCI{};
            shadowFrameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            shadowFrameBufferCI.pNext = nullptr;
            shadowFrameBufferCI.renderPass = mRenderPassShadow;
            shadowFrameBufferCI.attachmentCount = 1;
            shadowFrameBufferCI.pAttachments = attachmentViews;
            shadowFrameBufferCI.width = CurrentShadow->ShadowResolution;
            shadowFrameBufferCI.height = CurrentShadow->ShadowResolution;
            shadowFrameBufferCI.layers = 1;
            VK_CHECK_RESULT(vkCreateFramebuffer(m_pDevice->GetDevice(), &shadowFrameBufferCI, nullptr, &CurrentShadow->ShadowFrameBuffer))

            VkImageView ShadowSRV;
            CurrentShadow->ShadowMap.CreateSRV(&ShadowSRV);
            mShadowSRVPool.push_back(ShadowSRV);
        }
    }
}

void Renderer::OnRender(const UIState *pState, const Camera &camera, SwapChain *pSwapChain)
{
    mConstantBufferRing.OnBeginFrame();

    VkCommandBuffer cmdBuffer1 = mCommandListRing.GetNewCommandList();

    VkCommandBufferBeginInfo cmdBuffer1BI{};
    cmdBuffer1BI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBuffer1BI.pNext = nullptr;
    cmdBuffer1BI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdBuffer1BI.pInheritanceInfo = nullptr;
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer1, &cmdBuffer1BI))

    mGPUTimer.OnBeginFrame(cmdBuffer1, &mTimeStamps);

    PerFrame * pPerFrame = nullptr;
    if (m_pGLTFTexturesAndBuffers)
    {
        pPerFrame = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->SetPerFrameData(camera);

        pPerFrame->mInvScreenResolution[0] = 1.0f / ((float)mWidth);
        pPerFrame->mInvScreenResolution[1] = 1.0f / ((float)mHeight);
        pPerFrame->mLODBias = 0.0f;
        m_pGLTFTexturesAndBuffers->SetPerFrameConstants();
        m_pGLTFTexturesAndBuffers->SetSkinningMatricesForSkeletons();
    }

    // Render all shadow maps
    if (m_pGLTFDepthPass && pPerFrame != nullptr)
    {
        SetPerfMarkerBegin(cmdBuffer1, "ShadowPass");

        VkClearValue depthClearVal[1];
        depthClearVal[0].depthStencil.depth = 1.0f;
        depthClearVal[0].depthStencil.stencil = 0;

        VkRenderPassBeginInfo shadowRenderPassBI{};
        shadowRenderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        shadowRenderPassBI.pNext = nullptr;
        shadowRenderPassBI.renderPass = mRenderPassShadow;
        shadowRenderPassBI.renderArea.offset.x = 0;
        shadowRenderPassBI.renderArea.offset.y = 0;
        shadowRenderPassBI.clearValueCount = 1;
        shadowRenderPassBI.pClearValues = depthClearVal;

        auto ShadowMap = mShadowMapPool.begin();
        while (ShadowMap < mShadowMapPool.end())
        {
            // Clear shadow map
            shadowRenderPassBI.framebuffer = ShadowMap->ShadowFrameBuffer;
            shadowRenderPassBI.renderArea.extent.width = ShadowMap->ShadowResolution;
            shadowRenderPassBI.renderArea.extent.height = ShadowMap->ShadowResolution;
            vkCmdBeginRenderPass(cmdBuffer1, &shadowRenderPassBI, VK_SUBPASS_CONTENTS_INLINE);

            // Render to shadow map
            SetViewportAndScissor(cmdBuffer1, 0, 0, ShadowMap->ShadowResolution, ShadowMap->ShadowResolution);

            // Set per frame constant buffer values
            GLTFDepthPass::PerFrame* cbPerFrame = m_pGLTFDepthPass->SetPerFrameConstants();
            cbPerFrame->mViewProj = pPerFrame->mLights[ShadowMap->LightIndex].mLightViewProj;

            m_pGLTFDepthPass->Draw(cmdBuffer1);

            mGPUTimer.GetTimeStamp(cmdBuffer1, "Shadow Map Render");

            vkCmdEndRenderPass(cmdBuffer1);
            ++ShadowMap;
        }
        SetPerfMarkerEnd(cmdBuffer1);
    }

    // Render Scene to the GBuffer ------------------------------------------------
    SetPerfMarkerBegin(cmdBuffer1, "Color pass");
    VkRect2D renderArea = { 0, 0, mWidth, mHeight };
    if (pPerFrame && m_pGLTFBasePass)
    {
        std::vector<GLTFBaseMeshPass::BatchList> opaque, transparent;
        m_pGLTFBasePass->BuildBatchList(&opaque, &transparent);

        // Render opaque
        {
            mRenderPassFullGBufferWithClear.BeginPass(cmdBuffer1, renderArea);

            m_pGLTFBasePass->DrawBatchList(cmdBuffer1, &opaque);
            mGPUTimer.GetTimeStamp(cmdBuffer1, "PBR Opaque");

            mRenderPassFullGBufferWithClear.EndPass(cmdBuffer1);
        }

        // draw transparent geometry
        {
            mRenderPassFullGBuffer.BeginPass(cmdBuffer1, renderArea);

            std::sort(transparent.begin(), transparent.end());
            m_pGLTFBasePass->DrawBatchList(cmdBuffer1, &transparent);
            mGPUTimer.GetTimeStamp(cmdBuffer1, "PBR Transparent");

            mRenderPassFullGBuffer.EndPass(cmdBuffer1);
        }
    }
    else
    {
        mRenderPassFullGBufferWithClear.BeginPass(cmdBuffer1, renderArea);
        mRenderPassFullGBufferWithClear.EndPass(cmdBuffer1);
        mRenderPassJustDepthAndHDR.BeginPass(cmdBuffer1, renderArea);
        mRenderPassJustDepthAndHDR.EndPass(cmdBuffer1);
    }

    SetPerfMarkerEnd(cmdBuffer1);

    // Start tracking input/output resources at this point to handle HDR and SDR render paths
    VkImage      ImgCurrentInput  = mGBuffer.mHDR.Resource();
    VkImageView  SRVCurrentInput  = mGBuffer.mHDRSRV;

    // submit command buffer
    {
        VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer1));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer1;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = nullptr;
        VK_CHECK_RESULT(vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE))
    }

    // Wait for swapchain (we are going to render to it) -----------------------------------
    int imageIndex = pSwapChain->WaitForSwapChain();

    // Keep tracking input/output resource views
    ImgCurrentInput = mGBuffer.mHDR.Resource(); // these haven't changed, re-assign as sanity check
    SRVCurrentInput = mGBuffer.mHDRSRV;         // these haven't changed, re-assign as sanity check

    mCommandListRing.OnBeginFrame();

    VkCommandBuffer cmdBuffer2 = mCommandListRing.GetNewCommandList();

    VkCommandBufferBeginInfo cmdBuffer2BI{};
    cmdBuffer2BI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBuffer2BI.pNext = nullptr;
    cmdBuffer2BI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdBuffer2BI.pInheritanceInfo = nullptr;
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer2, &cmdBuffer2BI))

    SetPerfMarkerBegin(cmdBuffer2, "SwapChain RenderPass");

    VkRenderPassBeginInfo renderPassBI{};
    renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.pNext = nullptr;
    renderPassBI.renderPass = pSwapChain->GetRenderPass();
    renderPassBI.framebuffer = pSwapChain->GetFrameBuffer(imageIndex);
    renderPassBI.renderArea.offset.x = 0;
    renderPassBI.renderArea.offset.y = 0;
    renderPassBI.renderArea.extent.width = mWidth;
    renderPassBI.renderArea.extent.height = mHeight;
    renderPassBI.clearValueCount = 0;
    renderPassBI.pClearValues = nullptr;
    vkCmdBeginRenderPass(cmdBuffer2, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetScissor(cmdBuffer2, 0, 1, &mRectScissor);
    vkCmdSetViewport(cmdBuffer2, 0, 1, &mViewport);

    mImGUI.Draw(cmdBuffer2);
    mGPUTimer.GetTimeStamp(cmdBuffer2, "ImGUI Rendering");

    SetPerfMarkerEnd(cmdBuffer2);
    mGPUTimer.OnEndFrame();
    vkCmdEndRenderPass(cmdBuffer2);
    VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer2))

    VkSemaphore ImageAvailableSemaphore;
    VkSemaphore RenderFinishedSemaphores;
    VkFence CmdBufExecutedFences;
    pSwapChain->GetSemaphores(&ImageAvailableSemaphore, &RenderFinishedSemaphores, &CmdBufExecutedFences);

    VkPipelineStageFlags submitWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo2;
    submitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo2.pNext = nullptr;
    submitInfo2.waitSemaphoreCount = 1;
    submitInfo2.pWaitSemaphores = &ImageAvailableSemaphore;
    submitInfo2.pWaitDstStageMask = &submitWaitStage;
    submitInfo2.commandBufferCount = 1;
    submitInfo2.pCommandBuffers = &cmdBuffer2;
    submitInfo2.signalSemaphoreCount = 1;
    submitInfo2.pSignalSemaphores = &RenderFinishedSemaphores;

    VK_CHECK_RESULT(vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo2, CmdBufExecutedFences))

}
