#pragma once

#include "PCHVK.h"
#include "DeviceVK.h"
#include "Utilities/Benchmark.h"

namespace LeoVultana_VK
{
    // This class helps insert queries in the command buffer and readback the results.
    // The tricky part in fact is reading back the results without stalling the GPU.
    // For that it splits the readback heap in <numberOfBackBuffers> pieces, and it reads
    // from the last used chuck.
    class GPUTimeStamps
    {
    public:
        void OnCreate(Device *pDevice, uint32_t numberOfBackBuffers);
        void OnDestroy();

        void GetTimeStamp(VkCommandBuffer cmdBuffer, const char *label);
        void GetTimeStampUser(TimeStamp timeStamp);
        void OnBeginFrame(VkCommandBuffer cmdBuffer, std::vector<TimeStamp> *pTimestamps);
        void OnEndFrame();

    private:
        const uint32_t MaxValuesPerFrame = 128;

        Device*         m_pDevice;
        VkQueryPool     mQueryPool{};
        uint32_t        mFrame = 0;
        uint32_t        mNumberOfBackBuffers = 0;

        std::vector<std::string>    mLabels[5];
        std::vector<TimeStamp>      mCPUTimeStamps[5];
    };
}