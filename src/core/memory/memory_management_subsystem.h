//
// memory_management_subsystem.h
//
// Default IMemoryManagementSubsystem: owns an MPoolAllocator and publishes it
// to mnew / MObject::operator delete. Registered first by
// MEngineSubsystemRegistry::init(), so it is cleaned up last.
//

#ifndef METEOR_ENGINE_MEMORY_MANAGEMENT_SUBSYSTEM_H
#define METEOR_ENGINE_MEMORY_MANAGEMENT_SUBSYSTEM_H

#include <map>
#include <mutex>

#include "memory_subsystem_interface.h"

class IAllocator;

class MMemoryManagementSubsystem : public IMemoryManagementSubsystem
{
public:
    MMemoryManagementSubsystem() = default;
    ~MMemoryManagementSubsystem() override;

    void init() override;
    void cleanup() override;

    void* allocate(size_t size, size_t alignment, const std::type_info& type) override;
    void  deallocate(void* ptr) override;

    [[nodiscard]] SMemoryStats getStats() const override;

    void registerExternalReporter(const std::string& name, ExternalReporter reporter) override;
    void unregisterExternalReporter(const std::string& name) override;
    [[nodiscard]] std::vector<SExternalMemoryUsage> getExternalUsage() const override;

private:
    void logStats(const char* heading) const;

    IAllocator* allocator = nullptr;

    mutable std::mutex                      reportersMutex;
    std::map<std::string, ExternalReporter> reporters;
};

#endif // METEOR_ENGINE_MEMORY_MANAGEMENT_SUBSYSTEM_H
