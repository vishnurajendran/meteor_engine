#include "memory_management_subsystem.h"

#include "core/utils/logger.h"
#include "heap_tracker.h"
#include "mnew.h"
#include "pool_allocator.h"

MMemoryManagementSubsystem::~MMemoryManagementSubsystem()
{
    // cleanup() normally ran already; this covers a subsystem deleted without it.
    if (allocator)
        cleanup();
}

void MMemoryManagementSubsystem::init()
{
    if (allocator)
        return;

    allocator = new MPoolAllocator();
    MMemory::setRoutingAllocator(allocator);
    MMemory::setActiveSubsystem(this);

    // Everything else that goes through the global operator new.
    registerExternalReporter(MHeapTracker::REPORTER_NAME, &MHeapTracker::getUsage);

    MLOG(SString::format("MMemoryManagementSubsystem:: Initialised (page {0} KB, region {1} MB). "
                         "{2} MObjects were created on the heap before this point.",
                         MPageAllocator::PAGE_SIZE / 1024,
                         MPageAllocator::PAGE_SIZE * MPageAllocator::DEFAULT_PAGES_PER_REGION / (1024 * 1024),
                         MMemory::getHeapFallbackCount()));
}

void MMemoryManagementSubsystem::cleanup()
{
    if (!allocator)
        return;

    // From here on mnew uses the heap; deletes still reach the pool.
    if (MMemory::getActiveSubsystem() == this)
        MMemory::setActiveSubsystem(nullptr);

    logStats("MMemoryManagementSubsystem:: Final pool usage");

    {
        std::lock_guard lock(reportersMutex);
        reporters.clear();
    }

    const size_t live = allocator->getLiveAllocationCount();
    if (live == 0)
    {
        if (MMemory::getRoutingAllocator() == allocator)
            MMemory::setRoutingAllocator(nullptr);
        delete allocator;
        MLOG("MMemoryManagementSubsystem:: Pool released");
    }
    else
    {
        // Objects owned by statics or the application object are destroyed
        // after this. Keep the pool installed so their deletes still land in
        // it; the process exit reclaims the memory.
        const auto stats = allocator->getStats();
        for (const auto& pool : stats.pools)
        {
            if (pool.liveCount > 0)
                MWARN(SString::format("MMemoryManagementSubsystem:: still alive at shutdown: {0} x {1}",
                                      pool.liveCount, pool.typeName));
        }
        MWARN(SString::format("MMemoryManagementSubsystem:: {0} objects still alive; keeping the pool until exit", live));
    }
    allocator = nullptr;
}

void* MMemoryManagementSubsystem::allocate(const size_t size, const size_t alignment, const std::type_info& type)
{
    if (!allocator)
        return ::operator new(size); // not initialised; matches the heap path of deallocate
    return allocator->allocate(size, alignment, type);
}

void MMemoryManagementSubsystem::deallocate(void* ptr)
{
    MMemory::releaseObject(ptr);
}

SMemoryStats MMemoryManagementSubsystem::getStats() const
{
    return allocator ? allocator->getStats() : SMemoryStats{};
}

void MMemoryManagementSubsystem::registerExternalReporter(const std::string& name, ExternalReporter reporter)
{
    std::lock_guard lock(reportersMutex);
    reporters[name] = std::move(reporter);
}

void MMemoryManagementSubsystem::unregisterExternalReporter(const std::string& name)
{
    std::lock_guard lock(reportersMutex);
    reporters.erase(name);
}

std::vector<SExternalMemoryUsage> MMemoryManagementSubsystem::getExternalUsage() const
{
    std::lock_guard lock(reportersMutex);
    std::vector<SExternalMemoryUsage> usage;
    usage.reserve(reporters.size());
    for (const auto& [name, reporter] : reporters)
    {
        if (!reporter)
            continue;
        SExternalMemoryUsage entry = reporter();
        if (entry.name.empty())
            entry.name = name;
        usage.push_back(std::move(entry));
    }
    return usage;
}

void MMemoryManagementSubsystem::logStats(const char* heading) const
{
    if (!allocator)
        return;

    const auto stats = allocator->getStats();
    MLOG(SString::format("{0}: {1} live objects, {2} KB live, {3} pages in use, {4} regions ({5} MB reserved)",
                         heading, stats.liveAllocations, stats.liveBytes / 1024, stats.pagesInUse,
                         stats.regionCount, stats.reservedBytes / (1024 * 1024)));
    for (const auto& pool : stats.pools)
    {
        MVERBOSE(SString::format("    {0}: {1} live (peak {2}, total {3}), {4} B/slot, {5} pages{6}",
                                 pool.typeName, pool.liveCount, pool.peakLiveCount, pool.totalAllocations,
                                 pool.slotSize, pool.pageCount, pool.oversize ? " [oversize/heap]" : ""));
    }
}
