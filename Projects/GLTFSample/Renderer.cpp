#include "Renderer.h"
#include "UI.h"

//--------------------------------------------------------------------------------------
//
// OnCreate
//
//--------------------------------------------------------------------------------------
void Renderer::OnCreate(Device *pDevice, SwapChain *pSwapChain, float FontSize)
{
    m_pDevice = pDevice;

    // Initialize helpers

    // Create all the heaps for the resources views
    const uint32_t cbvDescriptorCount = 2000;
    const uint32_t srvDescriptorCount = 8000;
    const uint32_t uavDescriptorCount = 10;
    const uint32_t samplerDescriptorCount = 20;
    m_ResourceViewHeaps.OnCreate(pDevice, cbvDescriptorCount, srvDescriptorCount, uavDescriptorCount, samplerDescriptorCount);

    // Create a commandlist ring for the Direct queue
    uint32_t commandListsPerBackBuffer = 8;
    m_CommandListRing.OnCreate(pDevice, backBufferCount, commandListsPerBackBuffer);

    // Create a 'dynamic' constant buffer
    const uint32_t constantBuffersMemSize = 200 * 1024 * 1024;
    m_ConstantBufferRing.OnCreate(pDevice, backBufferCount, constantBuffersMemSize, "Uniforms");

    // Create a 'static' pool for vertices and indices
    const uint32_t staticGeometryMemSize = (1 * 128) * 1024 * 1024;
    m_VidMemBufferPool.OnCreate(pDevice, staticGeometryMemSize, true, "StaticGeom");

    // Create a 'static' pool for vertices and indices in system memory
    const uint32_t systemGeometryMemSize = 32 * 1024;
    m_SysMemBufferPool.OnCreate(pDevice, systemGeometryMemSize, false, "PostProcGeom");

    // initialize the GPU time stamps module
    m_GPUTimer.OnCreate(pDevice, backBufferCount);

    // Quick helper to upload resources, it has its own commandList and uses suballocation.
    const uint32_t uploadHeapMemSize = 1000 * 1024 * 1024;
    m_UploadHeap.OnCreate(pDevice, uploadHeapMemSize);    // initialize an upload heap (uses suballocation for faster results)

    // Create GBuffer and render passes
    //
    {
        m_GBuffer.OnCreate(
            pDevice,
            &m_ResourceViewHeaps,
            {
                { GBUFFER_DEPTH, VK_FORMAT_D32_SFLOAT},
                { GBUFFER_FORWARD, VK_FORMAT_R16G16B16A16_SFLOAT},
                { GBUFFER_MOTION_VECTORS, VK_FORMAT_R16G16_SFLOAT},
            },
            1
        );

        GBufferFlags fullGBuffer = GBUFFER_DEPTH | GBUFFER_FORWARD | GBUFFER_MOTION_VECTORS;
        bool bClear = true;
        m_RenderPassFullGBufferWithClear.OnCreate(&m_GBuffer, fullGBuffer, bClear,"m_RenderPassFullGBufferWithClear");
        m_RenderPassFullGBuffer.OnCreate(&m_GBuffer, fullGBuffer, !bClear, "m_RenderPassFullGBuffer");
        m_RenderPassJustDepthAndHdr.OnCreate(&m_GBuffer, GBUFFER_DEPTH | GBUFFER_FORWARD, !bClear, "m_RenderPassJustDepthAndHdr");
    }

    // Create render pass shadow, will clear contents
    {
        VkAttachmentDescription depthAttachments;
        AttachClearBeforeUse(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &depthAttachments);
        m_Render_pass_shadow = CreateRenderPassOptimal(m_pDevice->GetDevice(), 0, nullptr, &depthAttachments);
    }

    m_SkyDome.OnCreate(
        pDevice,
        m_RenderPassJustDepthAndHdr.GetRenderPass(),
        &m_UploadHeap, VK_FORMAT_R16G16B16A16_SFLOAT,
        &m_ResourceViewHeaps, &m_ConstantBufferRing,
        &m_VidMemBufferPool,
        "..\\Assets\\Textures\\EnvMaps\\diffuse.dds",
        "..\\Assets\\Textures\\EnvMaps\\specular.dds",
        VK_SAMPLE_COUNT_1_BIT);
    m_Wireframe.OnCreate(pDevice, m_RenderPassJustDepthAndHdr.GetRenderPass(), &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, VK_SAMPLE_COUNT_1_BIT);
    m_WireframeBox.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool);
    m_MagnifierPS.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, VK_FORMAT_R16G16B16A16_SFLOAT);

    // Initialize UI rendering resources
    m_ImGUI.OnCreate(m_pDevice, pSwapChain->GetRenderPass(), &m_UploadHeap, &m_ConstantBufferRing, FontSize);

    // Make sure upload heap has finished uploading before continuing
    m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
    m_UploadHeap.FlushAndFinish();
}

//--------------------------------------------------------------------------------------
//
// OnDestroy
//
//--------------------------------------------------------------------------------------
void Renderer::OnDestroy()
{
    m_AsyncPool.Flush();

    m_ImGUI.OnDestroy();
//    m_MagnifierPS.OnDestroy();
    m_WireframeBox.OnDestroy();
    m_Wireframe.OnDestroy();
//    m_SkyDome.OnDestroy();

    m_RenderPassFullGBufferWithClear.OnDestroy();
    m_RenderPassJustDepthAndHdr.OnDestroy();
    m_RenderPassFullGBuffer.OnDestroy();
    m_GBuffer.OnDestroy();

    vkDestroyRenderPass(m_pDevice->GetDevice(), m_Render_pass_shadow, nullptr);

    m_UploadHeap.OnDestroy();
    m_GPUTimer.OnDestroy();
    m_VidMemBufferPool.OnDestroy();
    m_SysMemBufferPool.OnDestroy();
    m_ConstantBufferRing.OnDestroy();
    m_ResourceViewHeaps.OnDestroy();
    m_CommandListRing.OnDestroy();
}

//--------------------------------------------------------------------------------------
//
// OnCreateWindowSizeDependentResources
//
//--------------------------------------------------------------------------------------
void Renderer::OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height)
{
    m_Width = Width;
    m_Height = Height;

    // Set the viewport
    //
    m_Viewport.x = 0;
    m_Viewport.y = (float)Height;
    m_Viewport.width = (float)Width;
    m_Viewport.height = -(float)(Height);
    m_Viewport.minDepth = (float)0.0f;
    m_Viewport.maxDepth = (float)1.0f;

    // Create scissor rectangle
    //
    m_RectScissor.extent.width = Width;
    m_RectScissor.extent.height = Height;
    m_RectScissor.offset.x = 0;
    m_RectScissor.offset.y = 0;

    // Create GBuffer
    //
    m_GBuffer.OnCreateWindowSizeDependentResources(pSwapChain, Width, Height);

    // Create frame buffers for the GBuffer render passes
    //
    m_RenderPassFullGBufferWithClear.OnCreateWindowSizeDependentResources(Width, Height);
    m_RenderPassJustDepthAndHdr.OnCreateWindowSizeDependentResources(Width, Height);
    m_RenderPassFullGBuffer.OnCreateWindowSizeDependentResources(Width, Height);

    // Update PostProcessing passes

    m_MagnifierPS.OnCreateWindowSizeDependentResources(&m_GBuffer.mHDR);
    m_bMagResourceReInit = true;
}

//--------------------------------------------------------------------------------------
//
// OnDestroyWindowSizeDependentResources
//
//--------------------------------------------------------------------------------------
void Renderer::OnDestroyWindowSizeDependentResources()
{
//    m_MagnifierPS.OnDestroyWindowSizeDependentResources();

    m_RenderPassFullGBufferWithClear.OnDestroyWindowSizeDependentResources();
    m_RenderPassJustDepthAndHdr.OnDestroyWindowSizeDependentResources();
    m_RenderPassFullGBuffer.OnDestroyWindowSizeDependentResources();
    m_GBuffer.OnDestroyWindowSizeDependentResources();
}

void Renderer::OnUpdateDisplayDependentResources(SwapChain *pSwapChain, bool bUseMagnifier)
{
    m_ImGUI.UpdatePipeline((pSwapChain->GetDisplayMode() == DISPLAYMODE_SDR) ? pSwapChain->GetRenderPass() : m_RenderPassJustDepthAndHdr.GetRenderPass());
}

//--------------------------------------------------------------------------------------
//
// OnUpdateLocalDimmingChangedResources
//
//--------------------------------------------------------------------------------------
void Renderer::OnUpdateLocalDimmingChangedResources(SwapChain *pSwapChain)
{

}

//--------------------------------------------------------------------------------------
//
// LoadScene
//
//--------------------------------------------------------------------------------------
int Renderer::LoadScene(GLTFCommon *pGLTFCommon, int Stage)
{
    // show loading progress
    //
    ImGui::OpenPopup("Loading");
    if (ImGui::BeginPopupModal("Loading", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        float progress = (float)Stage / 12.0f;
        ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), nullptr);
        ImGui::EndPopup();
    }

    // use multi threading
    AsyncPool *pAsyncPool = &m_AsyncPool;

    // Loading stages
    //
    if (Stage == 0)
    {
    }
    else if (Stage == 5)
    {
        Profile p("m_pGltfLoader->Load");

        m_pGLTFTexturesAndBuffers = new GLTFTexturesAndBuffers();
        m_pGLTFTexturesAndBuffers->OnCreate(m_pDevice, pGLTFCommon, &m_UploadHeap, &m_VidMemBufferPool, &m_ConstantBufferRing);
    }
    else if (Stage == 6)
    {
        Profile p("LoadTextures");

        // here we are loading onto the GPU all the textures and the inverse matrices
        // this data will be used to create the PBR and Depth passes
        m_pGLTFTexturesAndBuffers->LoadTextures(pAsyncPool);
    }
    else if (Stage == 7)
    {
        Profile p("m_GLTFDepth->OnCreate");

        //create the glTF's textures, VBs, IBs, shaders and descriptors for this particular pass
        m_GLTFDepth = new GLTFDepthPass();
        m_GLTFDepth->OnCreate(
            m_pDevice,
            m_Render_pass_shadow,
            &m_UploadHeap,
            &m_ResourceViewHeaps,
            &m_ConstantBufferRing,
            &m_VidMemBufferPool,
            m_pGLTFTexturesAndBuffers,
            pAsyncPool
        );

        m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
        m_UploadHeap.FlushAndFinish();
    }
    else if (Stage == 8)
    {
        Profile p("m_GLTFPBR->OnCreate");

        // same thing as above but for the PBR pass
        m_GLTFPBR = new GLTFPBRPass();
        m_GLTFPBR->OnCreate(
            m_pDevice,
            &m_UploadHeap,
            &m_ResourceViewHeaps,
            &m_ConstantBufferRing,
            &m_VidMemBufferPool,
            m_pGLTFTexturesAndBuffers,
            nullptr,
            false, // use SSAO mask
            m_ShadowSRVPool,
            &m_RenderPassFullGBufferWithClear,
            pAsyncPool
        );

        m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
        m_UploadHeap.FlushAndFinish();
    }
    else if (Stage == 9)
    {
        Profile p("m_GLTFBBox->OnCreate");

        // just a bounding box pass that will draw boundingboxes instead of the geometry itself
        m_GLTFBBox = new GLTFBBoxPass();
        m_GLTFBBox->OnCreate(
            m_pDevice,
            m_RenderPassJustDepthAndHdr.GetRenderPass(),
            &m_ResourceViewHeaps,
            &m_ConstantBufferRing,
            &m_VidMemBufferPool,
            m_pGLTFTexturesAndBuffers,
            &m_Wireframe
        );

        // we are borrowing the upload heap command list for uploading to the GPU the IBs and VBs
        m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
    }
    else if (Stage == 10)
    {
        Profile p("Flush");

        m_UploadHeap.FlushAndFinish();

        //once everything is uploaded we dont need the upload heaps anymore
        m_VidMemBufferPool.FreeUploadHeap();

        // tell caller that we are done loading the map
        return 0;
    }

    Stage++;
    return Stage;
}

//--------------------------------------------------------------------------------------
//
// UnloadScene
//
//--------------------------------------------------------------------------------------
void Renderer::UnloadScene()
{
    // wait for all the async loading operations to finish
    m_AsyncPool.Flush();

    m_pDevice->GPUFlush();

    if (m_GLTFPBR)
    {
        m_GLTFPBR->OnDestroy();
        delete m_GLTFPBR;
        m_GLTFPBR = nullptr;
    }

    if (m_GLTFDepth)
    {
        m_GLTFDepth->OnDestroy();
        delete m_GLTFDepth;
        m_GLTFDepth = nullptr;
    }

    if (m_GLTFBBox)
    {
        m_GLTFBBox->OnDestroy();
        delete m_GLTFBBox;
        m_GLTFBBox = nullptr;
    }

    if (m_pGLTFTexturesAndBuffers)
    {
        m_pGLTFTexturesAndBuffers->OnDestroy();
        delete m_pGLTFTexturesAndBuffers;
        m_pGLTFTexturesAndBuffers = nullptr;
    }

    assert(m_shadowMapPool.size() == m_ShadowSRVPool.size());
    while (!m_shadowMapPool.empty())
    {
        m_shadowMapPool.back().ShadowMap.OnDestroy();
        vkDestroyFramebuffer(m_pDevice->GetDevice(), m_shadowMapPool.back().ShadowFrameBuffer, nullptr);
        vkDestroyImageView(m_pDevice->GetDevice(), m_ShadowSRVPool.back(), nullptr);
        vkDestroyImageView(m_pDevice->GetDevice(), m_shadowMapPool.back().ShadowDSV, nullptr);
        m_ShadowSRVPool.pop_back();
        m_shadowMapPool.pop_back();
    }
}

void Renderer::AllocateShadowMaps(GLTFCommon* pGLTFCommon)
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
            m_shadowMapPool.push_back(ShadowInfo);
        }
    }

    if (NumShadows > MaxShadowInstances)
    {
        Trace("Number of shadows has exceeded maximum supported. Please grow value in gltfCommon.h/perFrameStruct.h");
        throw;
    }

    // If we had shadow information, allocate all required maps and bindings
    if (!m_shadowMapPool.empty())
    {
        std::vector<SceneShadowInfo>::iterator CurrentShadow = m_shadowMapPool.begin();
        for (uint32_t i = 0; CurrentShadow < m_shadowMapPool.end(); ++i, ++CurrentShadow)
        {
            CurrentShadow->ShadowMap.InitDepthStencil(m_pDevice, CurrentShadow->ShadowResolution, CurrentShadow->ShadowResolution, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, "ShadowMap");
            CurrentShadow->ShadowMap.CreateDSV(&CurrentShadow->ShadowDSV);

            // Create render pass shadow, will clear contents
            {
                VkAttachmentDescription depthAttachments;
                AttachClearBeforeUse(VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &depthAttachments);

                // Create frame buffer
                VkImageView attachmentViews[1] = { CurrentShadow->ShadowDSV };
                VkFramebufferCreateInfo fb_info = {};
                fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                fb_info.pNext = nullptr;
                fb_info.renderPass = m_Render_pass_shadow;
                fb_info.attachmentCount = 1;
                fb_info.pAttachments = attachmentViews;
                fb_info.width = CurrentShadow->ShadowResolution;
                fb_info.height = CurrentShadow->ShadowResolution;
                fb_info.layers = 1;
                VkResult res = vkCreateFramebuffer(m_pDevice->GetDevice(), &fb_info, nullptr, &CurrentShadow->ShadowFrameBuffer);
                assert(res == VK_SUCCESS);
            }

            VkImageView ShadowSRV;
            CurrentShadow->ShadowMap.CreateSRV(&ShadowSRV);
            m_ShadowSRVPool.push_back(ShadowSRV);
        }
    }
}

//--------------------------------------------------------------------------------------
//
// OnRender
//
//--------------------------------------------------------------------------------------
void Renderer::OnRender(const UIState* pState, const Camera& Cam, SwapChain* pSwapChain)
{
    // Let our resource managers do some house keeping
    m_ConstantBufferRing.OnBeginFrame();

    // command buffer calls
    VkCommandBuffer cmdBuf1 = m_CommandListRing.GetNewCommandList();

    {
        VkCommandBufferBeginInfo cmd_buf_info;
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = nullptr;
        cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmd_buf_info.pInheritanceInfo = nullptr;
        VkResult res = vkBeginCommandBuffer(cmdBuf1, &cmd_buf_info);
        assert(res == VK_SUCCESS);
    }

    m_GPUTimer.OnBeginFrame(cmdBuf1, &m_TimeStamps);

    // Sets the perFrame data
    PerFrame *pPerFrame = nullptr;
    if (m_pGLTFTexturesAndBuffers)
    {
        // fill as much as possible using the GLTF (camera, lights, ...)
        pPerFrame = m_pGLTFTexturesAndBuffers->m_pGLTFCommon->SetPerFrameData(Cam);

        // Set some lighting factors
        pPerFrame->iblFactor = pState->IBLFactor;
        pPerFrame->emissiveFactor = pState->EmissiveFactor;
        pPerFrame->invScreenResolution[0] = 1.0f / ((float)m_Width);
        pPerFrame->invScreenResolution[1] = 1.0f / ((float)m_Height);

        pPerFrame->wireframeOptions.setX(pState->WireframeColor[0]);
        pPerFrame->wireframeOptions.setY(pState->WireframeColor[1]);
        pPerFrame->wireframeOptions.setZ(pState->WireframeColor[2]);
        pPerFrame->wireframeOptions.setW(pState->WireframeMode == UIState::WireframeMode::WIREFRAME_MODE_SOLID_COLOR ? 1.0f : 0.0f);
        pPerFrame->lodBias = 0.0f;
        m_pGLTFTexturesAndBuffers->SetPerFrameConstants();
        m_pGLTFTexturesAndBuffers->SetSkinningMatricesForSkeletons();
    }

    // Render all shadow maps
    if (m_GLTFDepth && pPerFrame != nullptr)
    {
        SetPerfMarkerBegin(cmdBuf1, "ShadowPass");

        VkClearValue depth_clear_values[1];
        depth_clear_values[0].depthStencil.depth = 1.0f;
        depth_clear_values[0].depthStencil.stencil = 0;

        VkRenderPassBeginInfo rp_begin;
        rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_begin.pNext = nullptr;
        rp_begin.renderPass = m_Render_pass_shadow;
        rp_begin.renderArea.offset.x = 0;
        rp_begin.renderArea.offset.y = 0;
        rp_begin.clearValueCount = 1;
        rp_begin.pClearValues = depth_clear_values;

        std::vector<SceneShadowInfo>::iterator ShadowMap = m_shadowMapPool.begin();
        while (ShadowMap < m_shadowMapPool.end())
        {
            // Clear shadow map
            rp_begin.framebuffer = ShadowMap->ShadowFrameBuffer;
            rp_begin.renderArea.extent.width = ShadowMap->ShadowResolution;
            rp_begin.renderArea.extent.height = ShadowMap->ShadowResolution;
            vkCmdBeginRenderPass(cmdBuf1, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

            // Render to shadow map
            SetViewportAndScissor(cmdBuf1, 0, 0, ShadowMap->ShadowResolution, ShadowMap->ShadowResolution);

            // Set per frame constant buffer values
            GLTFDepthPass::PerFrame* cbPerFrame = m_GLTFDepth->SetPerFrameConstants();
            cbPerFrame->mViewProj = pPerFrame->lights[ShadowMap->LightIndex].mLightViewProj;

            m_GLTFDepth->Draw(cmdBuf1);

            m_GPUTimer.GetTimeStamp(cmdBuf1, "Shadow Map Render");

            vkCmdEndRenderPass(cmdBuf1);
            ++ShadowMap;
        }

        SetPerfMarkerEnd(cmdBuf1);
    }

    // Render Scene to the GBuffer ------------------------------------------------
    SetPerfMarkerBegin(cmdBuf1, "Color pass");

    VkRect2D renderArea = { 0, 0, m_Width, m_Height };

    if (pPerFrame != nullptr && m_GLTFPBR)
    {
        const bool bWireframe = pState->WireframeMode != UIState::WireframeMode::WIREFRAME_MODE_OFF;

        std::vector<GLTFPBRPass::BatchList> opaque, transparent;
        m_GLTFPBR->BuildBatchLists(&opaque, &transparent, bWireframe);

        // Render opaque
        {
            m_RenderPassFullGBufferWithClear.BeginPass(cmdBuf1, renderArea);
            m_GLTFPBR->DrawBatchList(cmdBuf1, &opaque, bWireframe);
            m_GPUTimer.GetTimeStamp(cmdBuf1, "PBR Opaque");

            m_RenderPassFullGBufferWithClear.EndPass(cmdBuf1);
        }

        // draw transparent geometry
        {
            m_RenderPassFullGBuffer.BeginPass(cmdBuf1, renderArea);

            std::sort(transparent.begin(), transparent.end());
            m_GLTFPBR->DrawBatchList(cmdBuf1, &transparent, bWireframe);
            m_GPUTimer.GetTimeStamp(cmdBuf1, "PBR Transparent");

            m_RenderPassFullGBuffer.EndPass(cmdBuf1);
        }

        // draw object's bounding boxes
        {
            m_RenderPassJustDepthAndHdr.BeginPass(cmdBuf1, renderArea);

            if (m_GLTFBBox)
            {
                if (pState->bDrawBoundingBoxes)
                {
                    m_GLTFBBox->Draw(cmdBuf1, pPerFrame->mCameraCurrViewProj);

                    m_GPUTimer.GetTimeStamp(cmdBuf1, "Bounding Box");
                }
            }

            // draw light's frustums
            if (pState->bDrawLightFrustum && pPerFrame != nullptr)
            {
                SetPerfMarkerBegin(cmdBuf1, "light frustums");

                math::Vector4 vCenter = math::Vector4(0.0f, 0.0f, 0.5f, 0.0f);
                math::Vector4 vRadius = math::Vector4(1.0f, 1.0f, 0.5f, 0.0f);
                math::Vector4 vColor = math::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
                for (uint32_t i = 0; i < pPerFrame->lightCount; i++)
                {
                    math::Matrix4 spotlightMatrix = math::inverse(pPerFrame->lights[i].mLightViewProj);
                    math::Matrix4 worldMatrix = pPerFrame->mCameraCurrViewProj * spotlightMatrix;
                    m_WireframeBox.Draw(cmdBuf1, &m_Wireframe, worldMatrix, vCenter, vRadius, vColor);
                }

                m_GPUTimer.GetTimeStamp(cmdBuf1, "Light's frustum");

                SetPerfMarkerEnd(cmdBuf1);
            }

            m_RenderPassJustDepthAndHdr.EndPass(cmdBuf1);
        }
    }
    else
    {
        m_RenderPassFullGBufferWithClear.BeginPass(cmdBuf1, renderArea);
        m_RenderPassFullGBufferWithClear.EndPass(cmdBuf1);
        m_RenderPassJustDepthAndHdr.BeginPass(cmdBuf1, renderArea);
        m_RenderPassJustDepthAndHdr.EndPass(cmdBuf1);
    }

    VkImageMemoryBarrier barrier[1] = {};
    barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier[0].pNext = nullptr;
    barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier[0].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier[0].subresourceRange.baseMipLevel = 0;
    barrier[0].subresourceRange.levelCount = 1;
    barrier[0].subresourceRange.baseArrayLayer = 0;
    barrier[0].subresourceRange.layerCount = 1;
    barrier[0].image = m_GBuffer.mHDR.Resource();
    vkCmdPipelineBarrier(cmdBuf1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, barrier);

    SetPerfMarkerEnd(cmdBuf1);

    // Post proc---------------------------------------------------------------------------

    // Magnifier Pass: m_HDR as input, pass' own output

    // Start tracking input/output resources at this point to handle HDR and SDR render paths
    VkImage      ImgCurrentInput  =  m_GBuffer.mHDR.Resource();
    VkImageView  SRVCurrentInput  =  m_GBuffer.mHDRSRV;

    // submit command buffer
    {
        VkResult res = vkEndCommandBuffer(cmdBuf1);
        assert(res == VK_SUCCESS);

        VkSubmitInfo submit_info;
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmdBuf1;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;
        res = vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
        assert(res == VK_SUCCESS);
    }

    // Wait for swapchain (we are going to render to it) -----------------------------------
    int imageIndex = pSwapChain->WaitForSwapChain();

    // Keep tracking input/output resource views
    ImgCurrentInput =  m_GBuffer.mHDR.Resource(); // these haven't changed, re-assign as sanity check
    SRVCurrentInput =  m_GBuffer.mHDRSRV;         // these haven't changed, re-assign as sanity check

    m_CommandListRing.OnBeginFrame();

    VkCommandBuffer cmdBuf2 = m_CommandListRing.GetNewCommandList();

    {
        VkCommandBufferBeginInfo cmd_buf_info;
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = nullptr;
        cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmd_buf_info.pInheritanceInfo = nullptr;
        VkResult res = vkBeginCommandBuffer(cmdBuf2, &cmd_buf_info);
        assert(res == VK_SUCCESS);
    }

    SetPerfMarkerBegin(cmdBuf2, "Swapchain RenderPass");

    // prepare render pass
    {
        VkRenderPassBeginInfo rp_begin = {};
        rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_begin.pNext = nullptr;
        rp_begin.renderPass = pSwapChain->GetRenderPass();
        rp_begin.framebuffer = pSwapChain->GetFrameBuffer(imageIndex);
        rp_begin.renderArea.offset.x = 0;
        rp_begin.renderArea.offset.y = 0;
        rp_begin.renderArea.extent.width = m_Width;
        rp_begin.renderArea.extent.height = m_Height;
        rp_begin.clearValueCount = 0;
        rp_begin.pClearValues = nullptr;
        vkCmdBeginRenderPass(cmdBuf2, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    }

    vkCmdSetScissor(cmdBuf2, 0, 1, &m_RectScissor);
    vkCmdSetViewport(cmdBuf2, 0, 1, &m_Viewport);

    // For SDR pipeline, we apply the tonemapping and then draw the GUI and skip the color conversion
    // Render HUD  -------------------------------------------------------------------------
    {
        m_ImGUI.Draw(cmdBuf2);
        m_GPUTimer.GetTimeStamp(cmdBuf2, "ImGUI Rendering");
    }

    SetPerfMarkerEnd(cmdBuf2);

    m_GPUTimer.OnEndFrame();

    vkCmdEndRenderPass(cmdBuf2);

    // Close & Submit the command list ----------------------------------------------------
    {
        VkResult res = vkEndCommandBuffer(cmdBuf2);
        assert(res == VK_SUCCESS);

        VkSemaphore ImageAvailableSemaphore;
        VkSemaphore RenderFinishedSemaphores;
        VkFence CmdBufExecutedFences;
        pSwapChain->GetSemaphores(&ImageAvailableSemaphore, &RenderFinishedSemaphores, &CmdBufExecutedFences);

        VkPipelineStageFlags submitWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info2;
        submit_info2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info2.pNext = nullptr;
        submit_info2.waitSemaphoreCount = 1;
        submit_info2.pWaitSemaphores = &ImageAvailableSemaphore;
        submit_info2.pWaitDstStageMask = &submitWaitStage;
        submit_info2.commandBufferCount = 1;
        submit_info2.pCommandBuffers = &cmdBuf2;
        submit_info2.signalSemaphoreCount = 1;
        submit_info2.pSignalSemaphores = &RenderFinishedSemaphores;

        res = vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submit_info2, CmdBufExecutedFences);
        assert(res == VK_SUCCESS);
    }
}
