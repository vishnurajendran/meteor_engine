#include "pool_allocator.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <new>

#if defined(__GNUG__)
#include <cxxabi.h>
#endif

std::string MPoolAllocator::demangle(const std::type_info& type)
{
#if defined(__GNUG__)
    int status = 0;
    char* readable = abi::__cxa_demangle(type.name(), nullptr, nullptr, &status);
    std::string result = (status == 0 && readable) ? readable : type.name();
    std::free(readable);
    return result;
#else
    // MSVC: "class MSpotLight" / "struct SFoo"
    std::string name = type.name();
    for (const char* prefix : { "class ", "struct " })
    {
        const std::string p(prefix);
        if (name.rfind(p, 0) == 0)
            return name.substr(p.size());
    }
    return name;
#endif
}

MPoolAllocator::MPoolAllocator(const size_t pagesPerRegion)
    : pageAllocator(pagesPerRegion)
{
}

MPoolAllocator::~MPoolAllocator()
{
    std::lock_guard lock(mutex);
    // Pools first: they hand their empty pages back to the page allocator,
    // which then frees every region (including pages that still hold objects;
    // the memory subsystem only destroys this allocator when nothing is live).
    pools.clear();
    for (auto& [ptr, alloc] : oversizeAllocations)
        ::operator delete(ptr, std::align_val_t(alloc.alignment));
    oversizeAllocations.clear();
    oversizeTypes.clear();
}

void* MPoolAllocator::allocate(const size_t size, const size_t alignment, const std::type_info& type)
{
    std::lock_guard lock(mutex);
    const std::type_index key(type);

    if (const auto it = pools.find(key); it != pools.end())
    {
        assert(it->second->getSlotSize() >= size && "same type, different size?");
        ++liveAllocations;
        return it->second->allocate();
    }

    if (const auto it = oversizeTypes.find(key); it == oversizeTypes.end()
        && MTypePool::slotsPerPageFor(size, alignment) > 0)
    {
        auto pool = std::make_unique<MTypePool>(pageAllocator, demangle(type), size, alignment);
        void* slot = pool->allocate();
        pools.emplace(key, std::move(pool));
        ++liveAllocations;
        return slot;
    }

    // Oversize: heap allocation, tracked per type.
    auto& typeEntry = oversizeTypes[key];
    if (!typeEntry)
    {
        typeEntry = std::make_unique<SOversizeType>();
        typeEntry->name = demangle(type);
        typeEntry->size = size;
    }

    const size_t align = std::max(alignment, alignof(std::max_align_t));
    void* mem = ::operator new(size, std::align_val_t(align));
    oversizeAllocations.emplace(mem, SOversizeAllocation{ typeEntry.get(), align });

    ++typeEntry->liveCount;
    ++typeEntry->totalAllocations;
    typeEntry->peakLiveCount = std::max(typeEntry->peakLiveCount, typeEntry->liveCount);
    ++liveAllocations;
    return mem;
}

bool MPoolAllocator::deallocate(void* ptr)
{
    if (!ptr)
        return true;

    std::lock_guard lock(mutex);

    if (pageAllocator.owns(ptr))
    {
        auto* page = static_cast<SPageHeader*>(MPageAllocator::pageOf(ptr));
        assert(page->magic == SPageHeader::MAGIC && "freeing into a page that is not in use (double delete?)");
        if (page->magic != SPageHeader::MAGIC)
            return true; // already returned; do not hand it to the heap
        page->pool->deallocate(ptr, page);
        --liveAllocations;
        return true;
    }

    if (const auto it = oversizeAllocations.find(ptr); it != oversizeAllocations.end())
    {
        --it->second.type->liveCount;
        ::operator delete(ptr, std::align_val_t(it->second.alignment));
        oversizeAllocations.erase(it);
        --liveAllocations;
        return true;
    }

    return false;
}

bool MPoolAllocator::owns(const void* ptr) const
{
    std::lock_guard lock(mutex);
    return pageAllocator.owns(ptr) || oversizeAllocations.contains(const_cast<void*>(ptr));
}

size_t MPoolAllocator::getLiveAllocationCount() const
{
    std::lock_guard lock(mutex);
    return liveAllocations;
}

SMemoryStats MPoolAllocator::getStats() const
{
    std::lock_guard lock(mutex);

    SMemoryStats stats;
    stats.pageSize        = MPageAllocator::PAGE_SIZE;
    stats.regionCount     = pageAllocator.getRegionCount();
    stats.reservedBytes   = pageAllocator.getReservedBytes();
    stats.pagesInUse      = pageAllocator.getPagesInUse();
    stats.freePages       = pageAllocator.getFreePageCount();
    stats.liveAllocations = liveAllocations;

    stats.pools.reserve(pools.size() + oversizeTypes.size());
    for (const auto& [key, pool] : pools)
    {
        stats.pools.push_back(pool->getStats());
        stats.liveBytes += pool->getLiveCount() * pool->getSlotSize();
    }

    for (const auto& [key, type] : oversizeTypes)
    {
        SMemoryPoolStats s;
        s.typeName         = type->name;
        s.slotSize         = type->size;
        s.liveCount        = type->liveCount;
        s.peakLiveCount    = type->peakLiveCount;
        s.totalAllocations = type->totalAllocations;
        s.oversize         = true;
        stats.pools.push_back(s);
        stats.oversizeBytes += type->liveCount * type->size;
    }
    stats.liveBytes += stats.oversizeBytes;

    std::sort(stats.pools.begin(), stats.pools.end(), [](const auto& a, const auto& b)
    {
        return a.liveCount * a.slotSize > b.liveCount * b.slotSize;
    });
    return stats;
}
