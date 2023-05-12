//
// Created by Administrator on 2023/5/12.
//

#include "PCH.h"
#include "AsyncCache.h"
#include "Async.h"
#include "Misc.h"

int Sync::Inc() {
    std::unique_lock<std::mutex> lock(mMutex);
    mCount++;
    return mCount;
}

int Sync::Dec() {
    std::unique_lock<std::mutex> lock(mMutex);
    mCount--;
    if (mCount == 0)
        mCondition.notify_all();
    return mCount;
}

int Sync::Get() {
    std::unique_lock<std::mutex> lock(mMutex);
    return mCount;
}

void Sync::Reset() {
    std::unique_lock<std::mutex> lock(mMutex);
    mCount = 0;
    mCondition.notify_all();
}

void Sync::Wait() {
    std::unique_lock<std::mutex> lock(mMutex);
    while (mCount != 0)
        mCondition.wait(lock);
}

Async::Async(std::function<void()> job, Sync *pSync) : mJob{job}, m_pSync{pSync}
{
    if (m_pSync)
        m_pSync->Inc();

    {
        std::unique_lock<std::mutex> lock(sMutex);

        while (sActiveThreads >= sMaxThreads)
        {
            sCondition.wait(lock);
        }

        sActiveThreads++;
    }

    m_pThread = new std::thread(
        [this]()
        {
            mJob();
            {
                std::lock_guard<std::mutex> lock(sMutex);
                sActiveThreads--;
            }
            sCondition.notify_one();
            if (m_pSync)
                m_pSync->Dec();
        });
}

Async::~Async() {
    m_pThread->join();
    delete m_pThread;
}

void Async::Wait(Sync *pSync) {
    if (pSync->Get() == 0)
        return;

    {
        std::lock_guard <std::mutex> lock(sMutex);
        sActiveThreads--;
    }

    sCondition.notify_one();

    pSync->Wait();

    {
        std::unique_lock<std::mutex> lock(sMutex);

        sCondition.wait(lock, []
        {
            return s_bExiting || (sActiveThreads<sMaxThreads);
        });

        sActiveThreads++;
    }
}

AsyncPool::~AsyncPool() {
    Flush();
}

void AsyncPool::Flush() {
    for (int i = 0; i < mPool.size(); i++)
        delete mPool[i];
    mPool.clear();
}

void AsyncPool::AddAsyncTask(std::function<void()> job, Sync *pSync) {
    mPool.push_back( new Async(job, pSync) );
}

void ExecAsyncIfThereIsAPool(AsyncPool *pAsyncPool, std::function<void()> job)
{
    // use MT if there is a pool
    if (pAsyncPool)
    {
        pAsyncPool->AddAsyncTask(job);
    }
    else
    {
        job();
    }
}

//
// Some static functions
//
int Async::sActiveThreads = 0;
std::mutex Async::sMutex;
std::condition_variable Async::sCondition;
bool Async::s_bExiting = false;
int Async::sMaxThreads = std::thread::hardware_concurrency();