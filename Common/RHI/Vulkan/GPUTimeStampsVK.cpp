#include "GPUTimeStampsVK.h"
#include "HelperVK.h"

using namespace LeoVultana_VK;

void GPUTimeStamps::OnCreate(Device *pDevice, uint32_t numberOfBackBuffers)
{
    m_pDevice = pDevice;
    mNumberOfBackBuffers = numberOfBackBuffers;
    mFrame = 0;

    const VkQueryPoolCreateInfo queryPoolCreateInfo =
    {
        VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        nullptr,
        (VkQueryPoolCreateFlags)0,
        VK_QUERY_TYPE_TIMESTAMP,
        MaxValuesPerFrame * numberOfBackBuffers,
        0,
    };
    VK_CHECK_RESULT(vkCreateQueryPool(pDevice->GetDevice(), &queryPoolCreateInfo, nullptr, &mQueryPool));
}

void GPUTimeStamps::OnDestroy()
{
    vkDestroyQueryPool(m_pDevice->GetDevice(), mQueryPool, nullptr);
    for (uint32_t i = 0; i < mNumberOfBackBuffers; i++) mLabels[i].clear();
}

void GPUTimeStamps::GetTimeStamp(VkCommandBuffer cmdBuffer, const char *label)
{
    auto measurements = (uint32_t)mLabels[mFrame].size();
    uint32_t offset = mFrame * MaxValuesPerFrame + measurements;

    vkCmdWriteTimestamp(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQueryPool, offset);
    mLabels[mFrame].emplace_back(label);
}

void GPUTimeStamps::GetTimeStampUser(TimeStamp timeStamp)
{
    mCPUTimeStamps[mFrame].emplace_back(timeStamp);
}

void GPUTimeStamps::OnBeginFrame(VkCommandBuffer cmdBuffer, std::vector<TimeStamp> *pTimestamps)
{
    std::vector<TimeStamp> &cpuTimeStamps = mCPUTimeStamps[mFrame];
    std::vector<std::string> &gpuLabels = mLabels[mFrame];

    pTimestamps->clear();
    pTimestamps->reserve(cpuTimeStamps.size() + gpuLabels.size());

    // copy CPU timestamps
    for (const auto & cpuTimeStamp : cpuTimeStamps)
    {
        pTimestamps->push_back(cpuTimeStamp);
    }

    // copy GPU timestamps
    uint32_t offset = mFrame * MaxValuesPerFrame;

    auto measurements = (uint32_t)gpuLabels.size();
    if (measurements > 0)
    {
        // timestampPeriod is the number of nanoseconds per timestamp value increment
        double microsecondsPerTick = (1e-3f * m_pDevice->GetPhysicalDeviceProperties().limits.timestampPeriod);
        {
            UINT64 TimingsInTicks[256] = {};
            VkResult res = vkGetQueryPoolResults(m_pDevice->GetDevice(), mQueryPool, offset, measurements, measurements * sizeof(UINT64), &TimingsInTicks, sizeof(UINT64), VK_QUERY_RESULT_64_BIT);
            if (res == VK_SUCCESS)
            {
                for (uint32_t i = 1; i < measurements; i++)
                {
                    TimeStamp ts = { mLabels[mFrame][i], float(microsecondsPerTick * (double)(TimingsInTicks[i] - TimingsInTicks[i - 1])) };
                    pTimestamps->push_back(ts);
                }

                // compute total
                TimeStamp ts = { "Total GPU Time", float(microsecondsPerTick * (double)(TimingsInTicks[measurements - 1] - TimingsInTicks[0])) };
                pTimestamps->push_back(ts);
            }
            else
            {
                pTimestamps->push_back({ "GPU counters are invalid", 0.0f });
            }
        }
    }

    vkCmdResetQueryPool(cmdBuffer, mQueryPool, offset, MaxValuesPerFrame);

    // we always need to clear these
    cpuTimeStamps.clear();
    gpuLabels.clear();

    GetTimeStamp(cmdBuffer, "Begin Frame");
}

void GPUTimeStamps::OnEndFrame()
{
    mFrame = (mFrame + 1) % mNumberOfBackBuffers;
}
