#include "PCHVK.h"
#include "DevicePropertiesVK.h"
#include "InstancePropertiesVK.h"

#define USE_VMA

#ifdef USE_VMA
#include "vulkan/vk_mem_alloc.h"
#endif

namespace LeoVultana_VK
{
    class Device
    {
    public:
        Device();
        ~Device();
        void OnCreate(const char* pAppName, const char* pEngineName, bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, HWND hWnd);
        void SetEssentialInstanceExtensions(bool cpuValidationLayerEnabled, bool gpuValidationLayerEnabled, InstanceProperties *pInstProp);
        void SetEssentialDeviceExtensions(DeviceProperties *pDeviceProp);
        void OnCreateEx(VkInstance vulkanInstance, VkPhysicalDevice physicalDevice, HWND hWnd, DeviceProperties *pDeviceProp);
        void OnDestroy();
        void GetDeviceInfo(std::string* deviceName, std::string* driverVersion);

        VkDevice GetDevice() { return mDevice; }
        VkPhysicalDevice GetPhysicalDevice() { return mPhysicalDevice; }

        VkQueue GetGraphicsQueue() { return mGraphicsQueue; }
        VkQueue GetPresentQueue() { return mPresentQueue; }
        VkQueue GetComputeQueue() { return mComputeQueue; }
        uint32_t GetGraphicsQueueFamilyIndex() const { return mGraphicsQueueFamilyIndex; }
        uint32_t GetPresentQueueFamilyIndex() const { return mPresentQueueFamilyIndex; }
        uint32_t GetComputeQueueFamilyIndex() const { return mComputeQueueFamilyIndex; }

        VkSurfaceKHR GetSurface() { return mSurface; }

#ifdef USE_VMA
        VmaAllocator GetAllocator() const { return m_hAllocator; }
#endif
        VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() { return mMemProperties; }
        VkPhysicalDeviceProperties GetPhysicalDeviceProperties() { return mDeviceProperties; }
        VkPhysicalDeviceSubgroupProperties GetPhysicalDeviceSubgroupProperties() { return mSubgroupProperties; }

        bool IsFp16Supported() { return mUsingFp16; };
        bool IsRT10Supported() { return mRt10Supported; }
        bool IsRT11Supported() { return mRt11Supported; }
        bool IsVRSTier1Supported() { return mVrs1Supported; }
        bool IsVRSTier2Supported() { return mVrs2Supported; }

        // Pipeline Cache
        void CreatePipelineCache();
        void DestroyPipelineCache();
        VkPipelineCache GetPipelineCache();

        void CreateShaderCache() {};
        void DestroyShaderCache() {};

        void GPUFlush();

    public:
        // pipeline cache
        VkPipelineCache mPipelineCache;

    private:
        VkInstance mInstance;
        VkDevice mDevice;
        VkPhysicalDevice mPhysicalDevice;
        VkPhysicalDeviceMemoryProperties mMemProperties;
        VkPhysicalDeviceProperties mDeviceProperties;
        VkPhysicalDeviceProperties2 mDeviceProperties2;
        VkPhysicalDeviceSubgroupProperties mSubgroupProperties;
        VkSurfaceKHR mSurface;

        VkQueue mPresentQueue;
        uint32_t mPresentQueueFamilyIndex;
        VkQueue mGraphicsQueue;
        uint32_t mGraphicsQueueFamilyIndex;
        VkQueue mComputeQueue;
        uint32_t mComputeQueueFamilyIndex;

        bool mUsingValidationLayer = false;
        bool mUsingFp16 = false;
        bool mRt10Supported = false;
        bool mRt11Supported = false;
        bool mVrs1Supported = false;
        bool mVrs2Supported = false;
#ifdef USE_VMA
        VmaAllocator m_hAllocator = nullptr;
#endif
    };
}