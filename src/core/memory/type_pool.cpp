#include "type_pool.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <new>

#include "page_allocator.h"

namespace
{
    constexpr size_t alignUp(const size_t value, const size_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    // Free slots store the next-free pointer in their first bytes.
    size_t minimumSlotAlignment(const size_t alignment)
    {
        return std::max(alignment, alignof(void*));
    }

#ifndef NDEBUG
    // Freed slots are filled with this in debug builds, so a use-after-free
    // reads obvious garbage (0xDDDDDDDD...) instead of the old object.
    constexpr unsigned char FREED_PATTERN = 0xDD;
#endif
}

size_t MTypePool::slotsPerPageFor(const size_t size, const size_t alignment)
{
    const size_t align  = minimumSlotAlignment(alignment);
    const size_t slot   = alignUp(std::max(size, sizeof(void*)), align);
    const size_t offset = alignUp(sizeof(SPageHeader), align);
    if (offset >= MPageAllocator::PAGE_SIZE)
        return 0;
    return (MPageAllocator::PAGE_SIZE - offset) / slot;
}

MTypePool::MTypePool(MPageAllocator& pageAllocator, std::string typeName, const size_t size, const size_t alignment)
    : pageAllocator(pageAllocator),
      typeName(std::move(typeName)),
      slotSize(alignUp(std::max(size, sizeof(void*)), minimumSlotAlignment(alignment))),
      slotAlignment(minimumSlotAlignment(alignment)),
      firstSlotOffset(alignUp(sizeof(SPageHeader), minimumSlotAlignment(alignment))),
      slotsPerPage(slotsPerPageFor(size, alignment))
{
    assert(slotsPerPage > 0 && "type too large for a page; use the oversize path");
    assert((slotAlignment & (slotAlignment - 1)) == 0 && "alignment must be a power of two");
    assert(slotAlignment <= MPageAllocator::PAGE_SIZE);
}

MTypePool::~MTypePool()
{
    // Only empty pages can be on the partial list without live objects; give
    // back what we can. Pages with live objects are owned by the page
    // allocator's regions and are freed when it is destroyed.
    while (partialHead)
    {
        SPageHeader* page = partialHead;
        unlinkPartial(page);
        if (page->liveCount == 0)
        {
            page->magic = 0;
            pageAllocator.releasePage(page);
            --pageCount;
        }
    }
}

SPageHeader* MTypePool::createPage()
{
    void* mem = pageAllocator.acquirePage();
    auto* page = new (mem) SPageHeader();
    page->pool      = this;
    page->capacity  = static_cast<uint32_t>(slotsPerPage);
    page->firstSlot = static_cast<char*>(mem) + firstSlotOffset;
    ++pageCount;
    return page;
}

void MTypePool::linkPartial(SPageHeader* page)
{
    assert(!page->inPartialList);
    page->prevPartial = nullptr;
    page->nextPartial = partialHead;
    if (partialHead)
        partialHead->prevPartial = page;
    partialHead = page;
    page->inPartialList = true;
}

void MTypePool::unlinkPartial(SPageHeader* page)
{
    assert(page->inPartialList);
    if (page->prevPartial)
        page->prevPartial->nextPartial = page->nextPartial;
    else
        partialHead = page->nextPartial;
    if (page->nextPartial)
        page->nextPartial->prevPartial = page->prevPartial;
    page->prevPartial = page->nextPartial = nullptr;
    page->inPartialList = false;
}

void* MTypePool::allocate()
{
    if (!partialHead)
        linkPartial(createPage());

    SPageHeader* page = partialHead;
    void* slot;

    if (page->freeList)
    {
        slot = page->freeList;
        page->freeList = *static_cast<void**>(slot);
    }
    else
    {
        assert(page->bumpIndex < page->capacity);
        slot = page->firstSlot + static_cast<size_t>(page->bumpIndex) * slotSize;
        ++page->bumpIndex;
    }

    ++page->liveCount;
    if (page->liveCount == page->capacity)
        unlinkPartial(page); // full

    ++liveCount;
    ++totalAllocations;
    peakLiveCount = std::max(peakLiveCount, liveCount);
    return slot;
}

void MTypePool::deallocate(void* ptr, SPageHeader* page)
{
    assert(page->pool == this);
    assert(page->liveCount > 0);
    assert(static_cast<char*>(ptr) >= page->firstSlot);
    assert((static_cast<size_t>(static_cast<char*>(ptr) - page->firstSlot) % slotSize) == 0
           && "pointer is not the start of a slot");

#ifndef NDEBUG
    std::memset(ptr, FREED_PATTERN, slotSize);
#endif

    const bool wasFull = page->liveCount == page->capacity;

    *static_cast<void**>(ptr) = page->freeList;
    page->freeList = ptr;
    --page->liveCount;
    --liveCount;

    if (wasFull)
        linkPartial(page);

    // Return an empty page unless it is the only page with room left, which
    // we keep so the next allocation does not immediately need a new page.
    if (page->liveCount == 0 && !(partialHead == page && page->nextPartial == nullptr))
    {
        unlinkPartial(page);
        page->magic = 0;
        pageAllocator.releasePage(page);
        --pageCount;
    }
}

SMemoryPoolStats MTypePool::getStats() const
{
    SMemoryPoolStats stats;
    stats.typeName         = typeName;
    stats.slotSize         = slotSize;
    stats.slotsPerPage     = slotsPerPage;
    stats.liveCount        = liveCount;
    stats.peakLiveCount    = peakLiveCount;
    stats.totalAllocations = totalAllocations;
    stats.pageCount        = pageCount;
    return stats;
}
