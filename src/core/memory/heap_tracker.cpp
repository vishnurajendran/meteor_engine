//
// heap_tracker.cpp
//
// Replacement global operator new / delete that count bytes. This must be
// the only translation unit in the executable that defines them.
//

#include "heap_tracker.h"

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <new>

#if defined(_WIN32)
#include <malloc.h>
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

namespace
{
    // Constant-initialised, so they work for allocations made by static
    // constructors that run before anything else.
    // Signed: a block allocated elsewhere but freed here subtracts without
    // ever having been added, so the live count can dip below the truth.
    std::atomic<int64_t> liveBytes{ 0 };
    std::atomic<int64_t> peakBytes{ 0 };
    std::atomic<int64_t> liveAllocations{ 0 };

    size_t usableSize(void* ptr)
    {
#if defined(_WIN32)
        return _msize(ptr);
#elif defined(__APPLE__)
        return malloc_size(ptr);
#else
        return malloc_usable_size(ptr);
#endif
    }

    size_t alignedUsableSize(void* ptr, [[maybe_unused]] const size_t alignment)
    {
#if defined(_WIN32)
        return _aligned_msize(ptr, alignment, 0);
#else
        return usableSize(ptr);
#endif
    }

    void onAllocated(const size_t size)
    {
        const int64_t now = liveBytes.fetch_add(static_cast<int64_t>(size), std::memory_order_relaxed)
                          + static_cast<int64_t>(size);
        liveAllocations.fetch_add(1, std::memory_order_relaxed);
        int64_t peak = peakBytes.load(std::memory_order_relaxed);
        while (now > peak && !peakBytes.compare_exchange_weak(peak, now, std::memory_order_relaxed)) {}
    }

    void onFreed(const size_t size)
    {
        liveBytes.fetch_sub(static_cast<int64_t>(size), std::memory_order_relaxed);
        liveAllocations.fetch_sub(1, std::memory_order_relaxed);
    }

    // Standard new loop: retry through the new_handler until it gives up.
    void* allocate(size_t size)
    {
        if (size == 0)
            size = 1;
        for (;;)
        {
            if (void* ptr = std::malloc(size))
            {
                onAllocated(usableSize(ptr));
                return ptr;
            }
            const std::new_handler handler = std::get_new_handler();
            if (!handler)
                return nullptr;
            handler();
        }
    }

    void* allocateAligned(size_t size, const size_t alignment)
    {
        if (size == 0)
            size = 1;
        for (;;)
        {
#if defined(_WIN32)
            void* ptr = _aligned_malloc(size, alignment);
#else
            // aligned_alloc wants a size that is a multiple of the alignment.
            void* ptr = std::aligned_alloc(alignment, (size + alignment - 1) / alignment * alignment);
#endif
            if (ptr)
            {
                onAllocated(alignedUsableSize(ptr, alignment));
                return ptr;
            }
            const std::new_handler handler = std::get_new_handler();
            if (!handler)
                return nullptr;
            handler();
        }
    }

    void release(void* ptr) noexcept
    {
        if (!ptr)
            return;
        onFreed(usableSize(ptr));
        std::free(ptr);
    }

    void releaseAligned(void* ptr, const size_t alignment) noexcept
    {
        if (!ptr)
            return;
        onFreed(alignedUsableSize(ptr, alignment));
#if defined(_WIN32)
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
    }
}

SExternalMemoryUsage MHeapTracker::getUsage()
{
    const auto clamp = [](const int64_t v) { return v > 0 ? static_cast<size_t>(v) : size_t{ 0 }; };

    SExternalMemoryUsage usage;
    usage.name            = REPORTER_NAME;
    usage.liveBytes       = clamp(liveBytes.load(std::memory_order_relaxed));
    usage.peakBytes       = clamp(peakBytes.load(std::memory_order_relaxed));
    usage.liveAllocations = clamp(liveAllocations.load(std::memory_order_relaxed));
    return usage;
}

// -- Replacement global operators ---------------------------------------------

void* operator new(const size_t size)
{
    if (void* ptr = allocate(size))
        return ptr;
    throw std::bad_alloc();
}

void* operator new[](const size_t size)
{
    return ::operator new(size);
}

void* operator new(const size_t size, const std::nothrow_t&) noexcept
{
    try { return allocate(size); }
    catch (...) { return nullptr; } // a new_handler may throw
}

void* operator new[](const size_t size, const std::nothrow_t& tag) noexcept
{
    return ::operator new(size, tag);
}

void operator delete(void* ptr) noexcept                                  { release(ptr); }
void operator delete[](void* ptr) noexcept                                { release(ptr); }
void operator delete(void* ptr, size_t) noexcept                          { release(ptr); }
void operator delete[](void* ptr, size_t) noexcept                        { release(ptr); }
void operator delete(void* ptr, const std::nothrow_t&) noexcept           { release(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) noexcept         { release(ptr); }

void* operator new(const size_t size, const std::align_val_t alignment)
{
    if (void* ptr = allocateAligned(size, static_cast<size_t>(alignment)))
        return ptr;
    throw std::bad_alloc();
}

void* operator new[](const size_t size, const std::align_val_t alignment)
{
    return ::operator new(size, alignment);
}

void* operator new(const size_t size, const std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    try { return allocateAligned(size, static_cast<size_t>(alignment)); }
    catch (...) { return nullptr; }
}

void* operator new[](const size_t size, const std::align_val_t alignment, const std::nothrow_t& tag) noexcept
{
    return ::operator new(size, alignment, tag);
}

void operator delete(void* ptr, const std::align_val_t alignment) noexcept           { releaseAligned(ptr, static_cast<size_t>(alignment)); }
void operator delete[](void* ptr, const std::align_val_t alignment) noexcept         { releaseAligned(ptr, static_cast<size_t>(alignment)); }
void operator delete(void* ptr, size_t, const std::align_val_t alignment) noexcept   { releaseAligned(ptr, static_cast<size_t>(alignment)); }
void operator delete[](void* ptr, size_t, const std::align_val_t alignment) noexcept { releaseAligned(ptr, static_cast<size_t>(alignment)); }
void operator delete(void* ptr, const std::align_val_t alignment, const std::nothrow_t&) noexcept   { releaseAligned(ptr, static_cast<size_t>(alignment)); }
void operator delete[](void* ptr, const std::align_val_t alignment, const std::nothrow_t&) noexcept { releaseAligned(ptr, static_cast<size_t>(alignment)); }
