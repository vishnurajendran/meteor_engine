//
// page_allocator.h
//
// Bottom layer of the pooled allocator. Requests big regions from the OS and
// hands them out as fixed-size pages to the type pools.
//
//   region 0 (16 MB)              region 1 (16 MB, added when region 0 ran out)
//   [page][page][page]...[page]   [page][page]...
//
// - Pages are PAGE_SIZE bytes and PAGE_SIZE-aligned, so any pointer inside a
//   page finds its page header with  ptr & ~(PAGE_SIZE - 1).
// - Growth adds a new region. Existing regions never move, so pointers to
//   pooled objects stay valid for their whole lifetime.
// - Not thread safe on its own; MPoolAllocator locks around every call.
//

#ifndef METEOR_ENGINE_PAGE_ALLOCATOR_H
#define METEOR_ENGINE_PAGE_ALLOCATOR_H

#include <cstddef>
#include <cstdint>
#include <vector>

class MPageAllocator
{
public:
    // Small pages keep rarely used types cheap: every type with a live object
    // holds at least one page. Types too big for a page use the oversize path.
    static constexpr size_t PAGE_SIZE   = 8 * 1024;
    static constexpr size_t REGION_SIZE = 16 * 1024 * 1024;
    static constexpr size_t DEFAULT_PAGES_PER_REGION = REGION_SIZE / PAGE_SIZE; // 2048

    explicit MPageAllocator(size_t pagesPerRegion = DEFAULT_PAGES_PER_REGION);
    ~MPageAllocator();

    MPageAllocator(const MPageAllocator&)            = delete;
    MPageAllocator& operator=(const MPageAllocator&) = delete;

    // Returns a PAGE_SIZE-aligned page. Adds a region if none are free.
    // Throws std::bad_alloc if the OS refuses.
    void* acquirePage();

    // Gives a page back. It is reused by the next acquirePage() of any pool.
    void releasePage(void* page);

    // True if `ptr` lies inside one of this allocator's regions.
    [[nodiscard]] bool owns(const void* ptr) const;

    static void* pageOf(const void* ptr)
    {
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) & ~(uintptr_t)(PAGE_SIZE - 1));
    }

    [[nodiscard]] size_t getRegionCount()   const { return regions.size(); }
    [[nodiscard]] size_t getPagesInUse()    const { return pagesInUse; }
    [[nodiscard]] size_t getFreePageCount() const { return freePages.size(); }
    [[nodiscard]] size_t getReservedBytes() const { return regions.size() * pagesPerRegion * PAGE_SIZE; }

private:
    struct SRegion
    {
        char* begin;
        char* end;
    };

    void addRegion();

    std::vector<SRegion> regions;
    std::vector<void*>   freePages;   // stack; back() is handed out next
    size_t               pagesPerRegion;
    size_t               pagesInUse = 0;
};

#endif // METEOR_ENGINE_PAGE_ALLOCATOR_H
