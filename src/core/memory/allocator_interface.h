//
// allocator_interface.h
//
// Abstract allocator. The memory subsystem owns one of these; nothing else in
// the engine should depend on the concrete type, so the implementation can be
// swapped (pooled, plain heap pass-through, debug/guarded, ...).
//

#ifndef METEOR_ENGINE_ALLOCATOR_INTERFACE_H
#define METEOR_ENGINE_ALLOCATOR_INTERFACE_H

#include <cstddef>
#include <typeinfo>

#include "memory_stats.h"

class IAllocator
{
public:
    virtual ~IAllocator() = default;

    // Returns memory for one object of `type`. `size` and `alignment` are
    // sizeof(T) / alignof(T). Never returns nullptr (throws std::bad_alloc).
    virtual void* allocate(size_t size, size_t alignment, const std::type_info& type) = 0;

    // Returns the memory to the allocator. The object must already be
    // destroyed. Returns false if `ptr` was not allocated here, so the caller
    // can hand it to the global heap instead.
    virtual bool deallocate(void* ptr) = 0;

    // True if `ptr` points into memory this allocator handed out.
    [[nodiscard]] virtual bool owns(const void* ptr) const = 0;

    [[nodiscard]] virtual size_t getLiveAllocationCount() const = 0;
    [[nodiscard]] virtual SMemoryStats getStats() const = 0;
};

#endif // METEOR_ENGINE_ALLOCATOR_INTERFACE_H
