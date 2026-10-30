#include "subsystem_registry.h"

#include "core/memory/memory_management_subsystem.h"
#include "core/utils/logger.h"

std::unordered_map<std::type_index, IEngineSubSystem*> MEngineSubsystemRegistry::subSystems;
std::vector<std::type_index> MEngineSubsystemRegistry::registrationOrder;

void MEngineSubsystemRegistry::init()
{
    MLOG("MEngineSubsystemRegistry:: Initialising registry");

    // Memory first: every MObject created through mnew<T>() after this point
    // comes from its pool, and since cleanup runs newest-first, memory is
    // cleaned up last.
    registerSubsystem<IMemoryManagementSubsystem, MMemoryManagementSubsystem>();
}

void MEngineSubsystemRegistry::cleanup()
{
    MLOG("MEngineSubsystemRegistry:: Cleaning registry");
    // Newest first: everything a subsystem uses was registered before it, so
    // it is still alive while that subsystem cleans up. Each one leaves the
    // map before it is deleted, so a lookup from a later cleanup gets nullptr
    // rather than a dangling pointer.
    const size_t total = registrationOrder.size();
    size_t curr = 0;
    for (auto it = registrationOrder.rbegin(); it != registrationOrder.rend(); ++it)
    {
        const auto found = subSystems.find(*it);
        if (found == subSystems.end())
            continue;

        IEngineSubSystem* inst = found->second;
        if (inst)
            inst->cleanup();
        subSystems.erase(found);
        delete inst;

        curr++;
        MLOG(SString::format("MEngineSubsystemRegistry:: {0} / {1} cleaned ({2})", curr, total, SString(it->name())));
    }
    MLOG("MEngineSubsystemRegistry:: Registry clean complete");
    subSystems.clear();
    registrationOrder.clear();
}