#pragma once

#include <cassert>
#include <cstdint>

// Ring Buffer，用来资源复用。例如Command Buffer
class Ring
{
public:

    void Create(uint32_t TotalSize)
    {
        mHead = 0;
        mAllocateSize = 0;
        mTotalSize = TotalSize;
    }

    uint32_t GetSize() const { return mAllocateSize; }
    uint32_t GetHead() const { return mHead; }
    uint32_t GetTail() const { return (mHead + mAllocateSize) % mTotalSize; }

    uint32_t PaddingToAvoidCrossOver(uint32_t size)
    {
        uint32_t tail = GetTail();
        if ((tail + size) > mTotalSize) return mTotalSize - tail;
        else return 0;
    }

    bool Alloc(uint32_t size, uint32_t* pOut)
    {
        if (mAllocateSize + size <= mTotalSize)
        {
            if (pOut) *pOut = GetTail();
            mAllocateSize += size;
            return true;
        }
        assert(false);
        return false;
    }

    bool Free(uint32_t size)
    {
        if (mAllocateSize >= size)
        {
            mHead = (mHead + size) % mTotalSize;
            mAllocateSize -= size;
            return true;
        }
        return false;
    }

private:
    uint32_t mHead;
    uint32_t mAllocateSize;
    uint32_t mTotalSize;
};

class RingWithTabs
{
    void OnCreate(uint32_t numberOfBackBuffers, uint32_t memTotalSize)
    {
        mBackBufferIndex = 0;
        mNumberOfBackBuffers = numberOfBackBuffers;

        //init mem per frame tracker
        mMemAllocatedInFrame = 0;
        for (int i = 0; i < 4; i++)
            mAllocatedMemPerBackBuffer[i] = 0;

        mMemory.Create(memTotalSize);
    }

    void OnDestroy()
    {
        mMemory.Free(mMemory.GetSize());
    }

    bool Alloc(uint32_t size, uint32_t *pOut)
    {
        uint32_t padding = mMemory.PaddingToAvoidCrossOver(size);
        if (padding > 0)
        {
            mMemAllocatedInFrame += padding;

            if (!mMemory.Alloc(padding, nullptr)) //alloc chunk to avoid crossover, ignore offset
            {
                return false;  //no mem, cannot allocate apdding
            }
        }

        if (mMemory.Alloc(size, pOut))
        {
            mMemAllocatedInFrame += size;
            return true;
        }
        return false;
    }

    void OnBeginFrame()
    {
        mAllocatedMemPerBackBuffer[mBackBufferIndex] = mMemAllocatedInFrame;
        mMemAllocatedInFrame = 0;

        mBackBufferIndex = (mBackBufferIndex + 1) % mNumberOfBackBuffers;

        // free all the entries for the oldest buffer in one go
        uint32_t memToFree = mAllocatedMemPerBackBuffer[mBackBufferIndex];
        mMemory.Free(memToFree);
    }

private:
    //internal ring buffer
    Ring mMemory;

    //this is the external ring buffer (I could have reused the Ring class though)
    uint32_t mBackBufferIndex;
    uint32_t mNumberOfBackBuffers;
    uint32_t mMemAllocatedInFrame;
    uint32_t mAllocatedMemPerBackBuffer[4];
};