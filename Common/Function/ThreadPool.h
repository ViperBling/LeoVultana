#pragma once

#include <functional>
#include <thread>
#include <deque>
#include <vector>
#include <map>
#include <condition_variable>

struct Task
{
    std::function<void()> m_job;
    std::vector<Task*> m_childTasks;
};

class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();
    void JobStealerLoop();
    void AddJob(std::function<void()> newJob);

private:
    bool bExiting;
    uint16_t mNumThreads;
    int mActiveThreads = 0;
    std::vector<std::thread> mPool;
    std::deque<Task> mQueue;
    std::condition_variable mCondition;
    std::mutex mQueueMutex;
};

ThreadPool* GetThreadPool();