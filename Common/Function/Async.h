#pragma once

#include "ThreadPool.h"

class Sync
{
public:
    int Inc();
    int Dec();
    int Get();
    void Reset();
    void Wait();
private:
    int mCount = 0;
    std::mutex mMutex;
    std::condition_variable mCondition;
};

class Async
{
public:
    Async(std::function<void()> job, Sync* pSync = nullptr);
    ~Async();
    static void Wait(Sync* pSync);

private:
    static int sActiveThreads;
    static int sMaxThreads;
    static std::mutex sMutex;
    static std::condition_variable sCondition;
    static bool s_bExiting;

    std::function<void()> mJob;
    Sync* m_pSync;
    std::thread *m_pThread;
};

class AsyncPool
{

public:
    ~AsyncPool();
    void Flush();
    void AddAsyncTask(std::function<void()> job, Sync *pSync = nullptr);

private:
    std::vector<Async *> mPool;
};

void ExecAsyncIfThereIsAPool(AsyncPool *pAsyncPool, std::function<void()> job);