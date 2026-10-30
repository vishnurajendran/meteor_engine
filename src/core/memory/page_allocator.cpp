#include "page_allocator.h"

#include <cassert>
#include <cstdlib>
#include <new>

#if defined(_WIN32)
#include <malloc.h>
#endif

namespace
{
    void* allocateAligned(size_t size, size_t alignment)
    {
#if defined(_WIN32)
        return _aligned_malloc(size, alignment);
#else
        return std::aligned_alloc(alignment, size); // size is a multiple of alignment
#endif
    }

    void freeAligned(void* ptr)
    {
#if defined(_WIN32)
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
    }
}

MPageAllocator::MPageAllocator(const size_t pagesPerRegion)
    : pagesPerRegion(pagesPerRegion > 0 ? pagesPerRegion : DEFAULT_PAGES_PER_REGION)
{
}

MPageAllocator::~MPageAllocator()
{
    for (const auto& region : regions)
        freeAligned(region.begin);
    regions.clear();
    freePages.clear();
}

void MPageAllocator::addRegion()
{
    const size_t bytes = pagesPerRegion * PAGE_SIZE;
    auto* mem = static_cast<char*>(allocateAligned(bytes, PAGE_SIZE));
    if (!mem)
        throw std::bad_alloc();

    regions.push_back({ mem, mem + bytes });

    // Push in reverse so the lowest page is handed out first.
    freePages.reserve(freePages.size() + pagesPerRegion);
    for (size_t i = pagesPerRegion; i-- > 0;)
        freePages.push_back(mem + i * PAGE_SIZE);
}

void* MPageAllocator::acquirePage()
{
    if (freePages.empty())
        addRegion();

    void* page = freePages.back();
    freePages.pop_back();
    ++pagesInUse;
    return page;
}

void MPageAllocator::releasePage(void* page)
{
    assert(page != nullptr && pageOf(page) == page && owns(page));
    assert(pagesInUse > 0);
    --pagesInUse;
    freePages.push_back(page);
}

bool MPageAllocator::owns(const void* ptr) const
{
    const auto* p = static_cast<const char*>(ptr);
    for (const auto& region : regions)
    {
        if (p >= region.begin && p < region.end)
            return true;
    }
    return false;
}
