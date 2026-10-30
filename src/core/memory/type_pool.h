//
// type_pool.h
//
// Middle layer of the pooled allocator: one pool per C++ type. A pool borrows
// pages from MPageAllocator and cuts each into equal slots of sizeof(T).
//
//   page (MPageAllocator::PAGE_SIZE, 8 KB)
//   [SPageHeader][slot][slot][slot]...[slot][unused tail]
//
// - Slots start at an offset aligned to the type's alignment and are
//   slotSize apart, and slotSize is a multiple of the alignment, so every slot
//   is correctly aligned.
// - Free slots in a page form a singly linked list stored inside the slots.
//   Allocation pops the head, deallocation pushes onto it: both O(1).
// - Pages that still have free slots are kept on a "partial" list. A page that
//   empties goes back to the page allocator, except the last one, which the
//   pool keeps so alternating new/delete does not thrash pages.
// - Not thread safe on its own; MPoolAllocator locks around every call.
//

#ifndef METEOR_ENGINE_TYPE_POOL_H
#define METEOR_ENGINE_TYPE_POOL_H

#include <cstddef>
#include <cstdint>
#include <string>

#include "memory_stats.h"

class MPageAllocator;
class MTypePool;

struct SPageHeader
{
    static constexpr uint32_t MAGIC = 0x4D54504Fu; // "MTPO"

    uint32_t     magic        = MAGIC;
    uint32_t     liveCount    = 0;  // slots in use
    uint32_t     bumpIndex    = 0;  // slots [0, bumpIndex) have been handed out at least once
    uint32_t     capacity     = 0;  // slots in this page
    MTypePool*   pool         = nullptr;
    void*        freeList     = nullptr; // previously used slots that are free again
    char*        firstSlot    = nullptr;
    SPageHeader* prevPartial  = nullptr;
    SPageHeader* nextPartial  = nullptr;
    bool         inPartialList = false;
};

class MTypePool
{
public:
    MTypePool(MPageAllocator& pageAllocator, std::string typeName, size_t size, size_t alignment);
    ~MTypePool();

    MTypePool(const MTypePool&)            = delete;
    MTypePool& operator=(const MTypePool&) = delete;

    // How many slots of this size fit in a page. 0 means the type is too big
    // for the pool and must be allocated another way.
    static size_t slotsPerPageFor(size_t size, size_t alignment);

    void* allocate();
    void  deallocate(void* ptr, SPageHeader* page);

    [[nodiscard]] const std::string& getTypeName() const { return typeName; }
    [[nodiscard]] size_t getSlotSize()     const { return slotSize; }
    [[nodiscard]] size_t getLiveCount()    const { return liveCount; }
    [[nodiscard]] SMemoryPoolStats getStats() const;

private:
    SPageHeader* createPage();
    void linkPartial(SPageHeader* page);
    void unlinkPartial(SPageHeader* page);

    MPageAllocator& pageAllocator;
    std::string     typeName;
    size_t          slotSize;
    size_t          slotAlignment;
    size_t          firstSlotOffset;
    size_t          slotsPerPage;

    SPageHeader*    partialHead = nullptr; // pages with at least one free slot
    size_t          pageCount   = 0;

    size_t liveCount        = 0;
    size_t peakLiveCount    = 0;
    size_t totalAllocations = 0;
};

#endif // METEOR_ENGINE_TYPE_POOL_H
