#include "mnew.h"

#include <atomic>
#include <cassert>

#include "allocator_interface.h"
#include "memory_subsystem_interface.h"

namespace
{
    std::atomic<IMemoryManagementSubsystem*> activeSubsystem{ nullptr };
    std::atomic<IAllocator*>                 routingAllocator{ nullptr };
    std::atomic<size_t>                      heapFallbackCount{ 0 };
}

namespace MMemory
{
    void setActiveSubsystem(IMemoryManagementSubsystem* subsystem)
    {
        activeSubsystem.store(subsystem);
    }

    IMemoryManagementSubsystem* getActiveSubsystem()
    {
        return activeSubsystem.load();
    }

    void setRoutingAllocator(IAllocator* allocator)
    {
        routingAllocator.store(allocator);
    }

    IAllocator* getRoutingAllocator()
    {
        return routingAllocator.load();
    }

    void* allocateObject(const size_t size, const size_t alignment, const std::type_info& type)
    {
        if (auto* subsystem = activeSubsystem.load())
            return subsystem->allocate(size, alignment, type);

        // Same call plain `new T` would make, so releaseObject's heap path
        // (::operator delete) matches it.
        assert(alignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__ && "over-aligned MObject before memory subsystem init");
        heapFallbackCount.fetch_add(1, std::memory_order_relaxed);
        return ::operator new(size);
    }

    void releaseObject(void* ptr)
    {
        if (!ptr)
            return;
        if (auto* allocator = routingAllocator.load(); allocator && allocator->deallocate(ptr))
            return;
        ::operator delete(ptr);
    }

    size_t getHeapFallbackCount()
    {
        return heapFallbackCount.load(std::memory_order_relaxed);
    }
}
