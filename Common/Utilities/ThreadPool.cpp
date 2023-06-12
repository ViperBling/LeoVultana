#include "PCH.h"
#include "ThreadPool.h"

static ThreadPool gThreadPool;

ThreadPool *GetThreadPool()
{
    return &gThreadPool;
}

#define ENABLE_MULTI_THREADING

ThreadPool::ThreadPool()
{
    mActiveThreads = 0;
#ifdef ENABLE_MULTI_THREADING
    mNumThreads = std::thread::hardware_concurrency();
    bExiting = false;
    for (int i = 0; i < mNumThreads; i++) {
        mPool.emplace_back(&ThreadPool::JobStealerLoop, GetThreadPool());
    }
#endif
}

ThreadPool::~ThreadPool()
{
#ifdef ENABLE_MULTI_THREADING
    bExiting = true;
    mCondition.notify_all();
    for (int ii = 0; ii < mNumThreads; ii++)
    {
        mPool[ii].join();
    }
#endif
}

void ThreadPool::JobStealerLoop()
{
#ifdef ENABLE_MULTI_THREADING
    while (true)
    {
        Task t;
        {
            std::unique_lock<std::mutex> lock(mQueueMutex);

            mCondition.wait(lock, [this] {return bExiting || (!mQueue.empty() && (mActiveThreads < mNumThreads)); });
            if (bExiting)
                return;
            mActiveThreads++;
            t = mQueue.front();
            mQueue.pop_front();
        }
        t.m_job();
        {
            std::unique_lock<std::mutex> lock(mQueueMutex);
            mActiveThreads--;
        }
    }
#endif
}

void ThreadPool::AddJob(std::function<void()> newJob)
{
#ifdef ENABLE_MULTI_THREADING
    if (!bExiting)
    {
        std::unique_lock<std::mutex> lock(mQueueMutex);

        Task t;
        t.m_job = newJob;

        mQueue.push_back(t);

        if (mActiveThreads < mNumThreads)
            mCondition.notify_one();
    }
#else
    newJob();
#endif
}
