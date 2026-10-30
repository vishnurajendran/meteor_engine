#include "jolt_memory_tracker.h"

#include <Jolt/Jolt.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <new>

#if defined(_WIN32)
#include <malloc.h>
#endif

namespace
{
    // Header in front of every block. 16 bytes keeps Jolt's default 16-byte
    // alignment for Allocate(); aligned blocks put it in the alignment padding.
    constexpr size_t HEADER_SIZE = 16;

    std::atomic<size_t> liveBytes{ 0 };
    std::atomic<size_t> peakBytes{ 0 };
    std::atomic<size_t> liveAllocations{ 0 };
    std::atomic<bool>   installed{ false };

    void onAllocated(const size_t size)
    {
        const size_t now = liveBytes.fetch_add(size, std::memory_order_relaxed) + size;
        liveAllocations.fetch_add(1, std::memory_order_relaxed);
        size_t peak = peakBytes.load(std::memory_order_relaxed);
        while (now > peak && !peakBytes.compare_exchange_weak(peak, now, std::memory_order_relaxed)) {}
    }

    void onFreed(const size_t size)
    {
        liveBytes.fetch_sub(size, std::memory_order_relaxed);
        liveAllocations.fetch_sub(1, std::memory_order_relaxed);
    }

    // -- Regular blocks: [size_t size | pad][user block] ------------------------

    void* trackedAllocate(const size_t size)
    {
        auto* raw = static_cast<char*>(std::malloc(size + HEADER_SIZE));
        if (!raw)
            return nullptr;
        std::memcpy(raw, &size, sizeof(size_t));
        onAllocated(size);
        return raw + HEADER_SIZE;
    }

    void trackedFree(void* block)
    {
        if (!block)
            return;
        char* raw = static_cast<char*>(block) - HEADER_SIZE;
        size_t size;
        std::memcpy(&size, raw, sizeof(size_t));
        onFreed(size);
        std::free(raw);
    }

    void* trackedReallocate(void* block, size_t /*oldSize*/, const size_t newSize)
    {
        if (!block)
            return trackedAllocate(newSize);

        char* raw = static_cast<char*>(block) - HEADER_SIZE;
        size_t oldSize;
        std::memcpy(&oldSize, raw, sizeof(size_t));

        auto* grown = static_cast<char*>(std::realloc(raw, newSize + HEADER_SIZE));
        if (!grown)
            return nullptr; // old block untouched, still counted

        std::memcpy(grown, &newSize, sizeof(size_t));
        onFreed(oldSize);
        onAllocated(newSize);
        return grown + HEADER_SIZE;
    }

    // -- Aligned blocks: [padding ... | size_t size | size_t align][user block] --

    void* trackedAlignedAllocate(const size_t size, const size_t alignment)
    {
        const size_t align = std::max(alignment, HEADER_SIZE);
#if defined(_WIN32)
        auto* raw = static_cast<char*>(_aligned_malloc(size + align, align));
#else
        const size_t total = (size + align + align - 1) / align * align;
        auto* raw = static_cast<char*>(std::aligned_alloc(align, total));
#endif
        if (!raw)
            return nullptr;
        char* block = raw + align;
        std::memcpy(block - 2 * sizeof(size_t), &size, sizeof(size_t));
        std::memcpy(block - sizeof(size_t), &align, sizeof(size_t));
        onAllocated(size);
        return block;
    }

    void trackedAlignedFree(void* block)
    {
        if (!block)
            return;
        auto* user = static_cast<char*>(block);
        size_t size, align;
        std::memcpy(&size, user - 2 * sizeof(size_t), sizeof(size_t));
        std::memcpy(&align, user - sizeof(size_t), sizeof(size_t));
        onFreed(size);
#if defined(_WIN32)
        _aligned_free(user - align);
#else
        std::free(user - align);
#endif
    }
}

namespace MJoltMemoryTracker
{
    void install()
    {
        // Once per process: blocks from a previous physics init must be freed
        // by these same functions.
        if (installed.exchange(true))
            return;

        JPH::Allocate        = trackedAllocate;
        JPH::Reallocate      = trackedReallocate;
        JPH::Free            = trackedFree;
        JPH::AlignedAllocate = trackedAlignedAllocate;
        JPH::AlignedFree     = trackedAlignedFree;
    }

    SExternalMemoryUsage getUsage()
    {
        SExternalMemoryUsage usage;
        usage.name            = "Jolt Physics";
        usage.liveBytes       = liveBytes.load(std::memory_order_relaxed);
        usage.peakBytes       = peakBytes.load(std::memory_order_relaxed);
        usage.liveAllocations = liveAllocations.load(std::memory_order_relaxed);
        return usage;
    }
}
