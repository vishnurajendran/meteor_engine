//
// memory_subsystem_interface.h
//
// Engine-facing memory API. Code asks the subsystem registry for this
// interface (or uses mnew<T>() from core/memory/mnew.h) and never sees the
// allocator implementation behind it.
//
//   auto* memory = MEngineSubsystemRegistry::getSubsystem<IMemoryManagementSubsystem>();
//   SMemoryStats stats = memory->getStats();
//

#ifndef METEOR_ENGINE_MEMORY_SUBSYSTEM_INTERFACE_H
#define METEOR_ENGINE_MEMORY_SUBSYSTEM_INTERFACE_H

#include <cstddef>
#include <functional>
#include <string>
#include <typeinfo>
#include <vector>

#include "core/engine/subsystem/subsystem_interface.h"
#include "memory_stats.h"

class IMemoryManagementSubsystem : public IEngineSubSystem
{
public:
    // Raw memory for one object of `type`. Prefer mnew<T>(), which also
    // constructs the object and checks T is an MObject.
    virtual void* allocate(size_t size, size_t alignment, const std::type_info& type) = 0;

    // Returns memory from allocate(). The object must already be destroyed.
    // (Named deallocate because `delete` is a C++ keyword.) For MObjects,
    // plain `delete obj` already ends up here through MObject::operator delete.
    virtual void deallocate(void* ptr) = 0;

    [[nodiscard]] virtual SMemoryStats getStats() const = 0;

    // Systems with their own allocators (Jolt, miniaudio, ...) report usage
    // here so it shows up next to the pool. The callback runs on whichever
    // thread asks for the stats, so it must be thread safe.
    using ExternalReporter = std::function<SExternalMemoryUsage()>;
    virtual void registerExternalReporter(const std::string& name, ExternalReporter reporter) = 0;
    virtual void unregisterExternalReporter(const std::string& name) = 0;
    [[nodiscard]] virtual std::vector<SExternalMemoryUsage> getExternalUsage() const = 0;
};

#endif // METEOR_ENGINE_MEMORY_SUBSYSTEM_INTERFACE_H
