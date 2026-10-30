//
// pool_allocator.h
//
// The engine's pooled IAllocator: a page allocator shared by one type pool per
// C++ type.
//
//   mnew<MSpotLight>()  ->  pool for MSpotLight  ->  page  ->  region
//   delete spotLight    ->  page header (ptr & ~(PAGE_SIZE-1))  ->  its pool
//
// Types too big for a page ("oversize") are allocated on the global heap but
// still counted per type, so the stats stay complete.
//
// Thread safe: one mutex guards everything. Allocation and deallocation of
// MObjects is not hot enough in this engine for that to matter yet; per-pool
// locks can come later.
//

#ifndef METEOR_ENGINE_POOL_ALLOCATOR_H
#define METEOR_ENGINE_POOL_ALLOCATOR_H

#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>

#include "allocator_interface.h"
#include "page_allocator.h"
#include "type_pool.h"

class MPoolAllocator : public IAllocator
{
public:
    explicit MPoolAllocator(size_t pagesPerRegion = MPageAllocator::DEFAULT_PAGES_PER_REGION);
    ~MPoolAllocator() override;

    MPoolAllocator(const MPoolAllocator&)            = delete;
    MPoolAllocator& operator=(const MPoolAllocator&) = delete;

    void* allocate(size_t size, size_t alignment, const std::type_info& type) override;
    bool  deallocate(void* ptr) override;
    [[nodiscard]] bool owns(const void* ptr) const override;

    [[nodiscard]] size_t getLiveAllocationCount() const override;
    [[nodiscard]] SMemoryStats getStats() const override;

    // Readable name for a type ("MSpotLight" rather than "10MSpotLight").
    static std::string demangle(const std::type_info& type);

private:
    struct SOversizeType
    {
        std::string name;
        size_t      size             = 0;
        size_t      liveCount        = 0;
        size_t      peakLiveCount    = 0;
        size_t      totalAllocations = 0;
    };

    struct SOversizeAllocation
    {
        SOversizeType* type;
        size_t         alignment;
    };

    mutable std::mutex mutex;
    MPageAllocator     pageAllocator;

    std::unordered_map<std::type_index, std::unique_ptr<MTypePool>>     pools;
    std::unordered_map<std::type_index, std::unique_ptr<SOversizeType>> oversizeTypes;
    std::unordered_map<void*, SOversizeAllocation>                      oversizeAllocations;

    size_t liveAllocations = 0;
};

#endif // METEOR_ENGINE_POOL_ALLOCATOR_H
