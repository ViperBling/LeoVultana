#pragma once

#include "DeviceVK.h"
#include "UploadHeapVK.h"
#include "ResourceViewHeapsVK.h"
#include "Utilities/DDSLoader.h"
#include "Vulkan/vk_mem_alloc.h"

namespace LeoVultana_VK
{
    // Create 2D Texture or Render Target
    class Texture
    {
    public:
        Texture();
        virtual ~Texture();
        virtual void OnDestroy();

        // load file into heap
        INT32 Init(Device *pDevice, VkImageCreateInfo *pCreateInfo, const char* name = nullptr);
        INT32 InitRenderTarget(
            Device *pDevice, uint32_t width, uint32_t height, VkFormat format,
            VkSampleCountFlagBits msaa, VkImageUsageFlags usage,
            bool bUAV, const char* name = nullptr,
            VkImageCreateFlagBits flags = (VkImageCreateFlagBits)0);
        INT32 InitDepthStencil(
            Device *pDevice, uint32_t width, uint32_t height, VkFormat format,
            VkSampleCountFlagBits msaa, const char* name = nullptr);
        bool InitFromFile(
            Device* pDevice, UploadHeap* pUploadHeap,
            const char *szFilename, bool useSRGB = false, VkImageUsageFlags usageFlags = 0, float cutOff = 1.0f);
        bool InitFromData(
            Device* pDevice, UploadHeap& uploadHeap,
            const IMG_INFO& header, const void* data, const char* name = nullptr);

        VkImage Resource() const { return mResource; }

        void CreateRTV(VkImageView *pImageView, int mipLevel = -1, VkFormat format = VK_FORMAT_UNDEFINED);
        void CreateSRV(VkImageView *pImageView, int mipLevel = -1);
        void CreateDSV(VkImageView *pImageView);
        void CreateCubeSRV(VkImageView *pImageView);

        uint32_t GetWidth() const { return mHeader.width; }
        uint32_t GetHeight() const { return mHeader.height; }
        uint32_t GetMipCount() const { return mHeader.mipMapCount; }
        uint32_t GetArraySize() const { return mHeader.arraySize; }
        VkFormat GetFormat() const { return mFormat; }

    protected:
        struct FootPrint
        {
            UINT8*      pixels;
            uint32_t    width, height, offset;
        } footPrints[6][12];

        VkImage CreateTextureCommitted(
            Device *pDevice, UploadHeap *pUploadHeap,
            const char *pName, bool useSRGB = false, VkImageUsageFlags usageFlags = 0);
        void LoadAndUpload(Device *pDevice, UploadHeap *pUploadHeap, ImgLoader *pDds, VkImage pTexture2D);
        bool isCubemap()const;

    private:
        Device*         m_pDevice{};
        std::string     mName{};
        VkFormat        mFormat;
        VkImage         mResource{};
        IMG_INFO        mHeader;
//#ifdef USE_VMA
        VmaAllocation   mImageAlloc{};
//#else
        VkDeviceMemory  mDeviceMemory{};
//#endif
    };


}