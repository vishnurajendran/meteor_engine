//
// memory_stats.h
//
// Plain data snapshots returned by the allocator and the memory subsystem.
// Copied out under the allocator lock, so they are safe to read on any thread.
//

#ifndef METEOR_ENGINE_MEMORY_STATS_H
#define METEOR_ENGINE_MEMORY_STATS_H

#include <cstddef>
#include <string>
#include <vector>

// One type pool (one MObject class).
struct SMemoryPoolStats
{
    std::string typeName;            // demangled C++ type name
    size_t      slotSize         = 0; // bytes per object slot (sizeof(T), rounded to alignment)
    size_t      slotsPerPage     = 0; // 0 for oversize types (they bypass pages)
    size_t      liveCount        = 0; // objects alive right now
    size_t      peakLiveCount    = 0; // highest liveCount seen
    size_t      totalAllocations = 0; // every allocation ever made from this pool
    size_t      pageCount        = 0; // pages this pool currently holds
    bool        oversize         = false; // too big for a page; allocated on the heap but still tracked
};

// The whole pooled allocator.
struct SMemoryStats
{
    size_t pageSize        = 0;
    size_t regionCount     = 0; // big blocks requested from the OS
    size_t reservedBytes   = 0; // regionCount * region size
    size_t pagesInUse      = 0; // pages handed to type pools
    size_t freePages       = 0; // pages waiting in the page allocator

    size_t liveAllocations = 0; // all live objects, pooled + oversize
    size_t liveBytes       = 0; // liveCount * slotSize summed over pools (+ oversize bytes)
    size_t oversizeBytes   = 0;

    std::vector<SMemoryPoolStats> pools;
};

// Memory owned by a system that does not use the pool (Jolt, miniaudio, ...).
// Reported through IMemoryManagementSubsystem::registerExternalReporter().
struct SExternalMemoryUsage
{
    std::string name;
    size_t      liveBytes       = 0;
    size_t      peakBytes       = 0;
    size_t      liveAllocations = 0;
};

#endif // METEOR_ENGINE_MEMORY_STATS_H
