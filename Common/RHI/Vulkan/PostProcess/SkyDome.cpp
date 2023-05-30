#include "PCHVK.h"
#include "SkyDome.h"
#include "HelperVK.h"
#include "ExtDebugUtilsVK.h"
#include "HelperVK.h"
#include "TextureVK.h"

using namespace LeoVultana_VK;

void SkyDome::OnCreate(
    Device *pDevice,
    VkRenderPass renderPass,
    UploadHeap *pUploadHeap,
    VkFormat outFormat,
    ResourceViewHeaps *pResourceViewHeaps,
    DynamicBufferRing *pDynamicBufferRing,
    StaticBufferPool *pStaticBufferPool,
    const char *pDiffuseCubeMap,
    const char *pSpecularCubeMap,
    VkSampleCountFlagBits sampleDescCount)
{
    m_pDevice = pDevice;
    m_pDynamicBufferRing = pDynamicBufferRing;
    m_pResourceViewHeaps = pResourceViewHeaps;

    mCubeDiffuseTexture.InitFromFile(pDevice, pUploadHeap, pDiffuseCubeMap, true); // SRGB
    mCubeSpecularTexture.InitFromFile(pDevice, pUploadHeap, pSpecularCubeMap, true);

    pUploadHeap->FlushAndFinish();

    mCubeDiffuseTexture.CreateCubeSRV(&mCubeDiffuseTextureView);
    mCubeSpecularTexture.CreateCubeSRV(&mCubeSpecularTextureView);

    VkSamplerCreateInfo diffuseSamplerCI{};
    diffuseSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    diffuseSamplerCI.magFilter = VK_FILTER_NEAREST;
    diffuseSamplerCI.minFilter = VK_FILTER_NEAREST;
    diffuseSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    diffuseSamplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    diffuseSamplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    diffuseSamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    diffuseSamplerCI.minLod = -1000;
    diffuseSamplerCI.maxLod = 1000;
    diffuseSamplerCI.maxAnisotropy = 1.0f;
    VK_CHECK_RESULT(vkCreateSampler(pDevice->GetDevice(), &diffuseSamplerCI, nullptr, &mSamplerDiffuseCube))

    VkSamplerCreateInfo specularSamplerCI{};
    specularSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    specularSamplerCI.magFilter = VK_FILTER_LINEAR;
    specularSamplerCI.minFilter = VK_FILTER_LINEAR;
    specularSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    specularSamplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    specularSamplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    specularSamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    specularSamplerCI.minLod = -1000;
    specularSamplerCI.maxLod = 1000;
    specularSamplerCI.maxAnisotropy = 1.0f;
    VK_CHECK_RESULT(vkCreateSampler(pDevice->GetDevice(), &specularSamplerCI, nullptr, &mSamplerSpecularCube));

    //create descriptor
    std::vector<VkDescriptorSetLayoutBinding> descSetLayoutBinding(2);
    descSetLayoutBinding[0].binding = 0;
    descSetLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descSetLayoutBinding[0].descriptorCount = 1;
    descSetLayoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    descSetLayoutBinding[0].pImmutableSamplers = nullptr;

    descSetLayoutBinding[1].binding = 1;
    descSetLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descSetLayoutBinding[1].descriptorCount = 1;
    descSetLayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descSetLayoutBinding[1].pImmutableSamplers = nullptr;

    m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&descSetLayoutBinding, &mDescSetLayout, &mDescSet);
    pDynamicBufferRing->SetDescriptorSet(0, sizeof(math::Matrix4), mDescSet);
    SetDescriptorSpecular(1, mDescSet);

    mSkyDome.OnCreate(
        pDevice, renderPass,
        "SkyDome.glsl",
        "main", "",
        pStaticBufferPool, pDynamicBufferRing,
        mDescSetLayout, nullptr, sampleDescCount);
}

void SkyDome::OnDestroy()
{
    mSkyDome.OnDestroy();
    vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), mDescSetLayout, nullptr);

    vkDestroySampler(m_pDevice->GetDevice(), mSamplerDiffuseCube, nullptr);
    vkDestroySampler(m_pDevice->GetDevice(), mSamplerSpecularCube, nullptr);

    vkDestroyImageView(m_pDevice->GetDevice(), mCubeDiffuseTextureView, nullptr);
    vkDestroyImageView(m_pDevice->GetDevice(), mCubeSpecularTextureView, nullptr);

    m_pResourceViewHeaps->FreeDescriptor(mDescSet);

    mCubeDiffuseTexture.OnDestroy();
    mCubeSpecularTexture.OnDestroy();
}

void SkyDome::Draw(VkCommandBuffer cmdBuffer, const math::Matrix4 &invViewProj)
{
    SetPerfMarkerBegin(cmdBuffer, "SkyDome Cube");

    math::Matrix4* cbPerDraw;
    VkDescriptorBufferInfo constantBuffer;
    m_pDynamicBufferRing->AllocateConstantBuffer(sizeof(math::Matrix4), (void **)&cbPerDraw, &constantBuffer);
    *cbPerDraw = invViewProj;

    mSkyDome.Draw(cmdBuffer, &constantBuffer, mDescSet);

    SetPerfMarkerEnd(cmdBuffer);
}

void SkyDome::GenerateDiffuseMapFromEnvironmentMap()
{
}

void SkyDome::SetDescriptorDiffuse(uint32_t index, VkDescriptorSet descriptorSet)
{
    SetDescriptorSet(m_pDevice->GetDevice(), index, mCubeDiffuseTextureView, &mSamplerDiffuseCube, descriptorSet);
}

void SkyDome::SetDescriptorSpecular(uint32_t index, VkDescriptorSet descriptorSet)
{
    SetDescriptorSet(m_pDevice->GetDevice(), index, mCubeSpecularTextureView, &mSamplerSpecularCube, descriptorSet);
}
