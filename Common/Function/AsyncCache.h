#pragma once

#include "ThreadPool.h"

// This is a multithreaded shader cache. This is how it works:
//
// Each shader compilation is invoked by an app thread using the Async class, this class executes the shader compilation in a new thread.
// To prevent context switches we need to limit the number of running threads to the number of cores.
// For that reason there is a global counter that keeps track of the number of running threads.
// This counter gets incremented when the thread is running a task and decremented when it finishes.
// It is also decremented when a thread is put into Wait mode and incremented when a thread is signaled AND there is a core available to resume the thread.
// If all cores are busy the app thread is put to Wait to prevent it from spawning more threads.
//
// When multiple threads attempt to compile the same shader it happens the following:
// 1) the thread that first comes gets to compile the shader
// 2) the rest of threads (let's say 'n') are put into _Wait_ mode
// 3) Since 'n' cores are now free, 'n' threads are resumed/spawned to keep executing other tasks
//
// This way all the cores should be plenty of work and thread context switches should be minimal.
//
//

#include "Async.h"

#define CACHE_ENABLE

template<typename T>
class Cache
{
public:
    struct CacheEntry
    {
        Sync mSync;
        T mData;
    };
    typedef std::map<size_t, CacheEntry> DatabaseType;

private:
    DatabaseType mDatabase;
    std::mutex mMutex;

public:
    bool CacheMiss(size_t hash, T *pOut)
    {
#ifdef CACHE_ENABLE
        typename DatabaseType::iterator it;

        // find whether the shader is in the cache, create an empty entry just so other threads know this thread will be compiling the shader
        {
            std::lock_guard<std::mutex> lock(mMutex);
            it = mDatabase.find(hash);

            // shader not found, we need to compile the shader!
            if (it == mDatabase.end())
            {
#ifdef CACHE_LOG
                Trace(format("thread 0x%04x Compi Begin: %p %i\n", GetCurrentThreadId(), hash, m_database[hash].m_Sync.Get()));
#endif
                // inc syncing object so other threads requesting this same shader can tell there is a compilation in progress and they need to wait for this thread to finish.
                mDatabase[hash].mSync.Inc();
                return true;
            }
        }

        // If we have seen these shaders before then:
        {
            // If there is a thread already trying to compile this shader then wait for that thread to finish
            if (it->second.mSync.Get() == 1)
            {
#ifdef CACHE_LOG
                Trace(format("thread 0x%04x Wait: %p %i\n", GetCurrentThreadId(), hash, it->second.mSync.Get()));
#endif
                Async::Wait(&it->second.m_Sync);
            }

            // if the shader was compiled then return it
            *pOut = it->second.mData;

#ifdef CACHE_LOG
            Trace(format("thread 0x%04x Was cache: %p \n", GetCurrentThreadId(), hash));
#endif
            return false;
        }
#endif
        return true;
    }

    void UpdateCache(size_t hash, T *pValue)
    {
#ifdef CACHE_ENABLE
        typename DatabaseType::iterator it;

        {
            std::lock_guard<std::mutex> lock(mMutex);
            it = mDatabase.find(hash);
            assert(it != mDatabase.end());
        }
#ifdef CACHE_LOG
        Trace(format("thread 0x%04x Compi End: %p %i\n", GetCurrentThreadId(), hash, it->second.mSync.Get()));
#endif
        it->second.mData = *pValue;
        //assert(it->second.m_Sync.Get() == 1);

        // The shader has been compiled, set sync to 0 to indicate it is compiled
        // This also wakes up all the threads waiting on  Async::Wait(&it->second.m_Sync);
        it->second.mSync.Dec();
#endif
    }

    template<typename Func>
    void ForEach(Func func)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        for(auto it = mDatabase.begin(); it != mDatabase.end(); ++it)
        {
            func(it);
        }
    }
};