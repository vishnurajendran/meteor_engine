#include "subsystem_registry.h"

#include "core/utils/logger.h"

std::unordered_map<std::type_index, IEngineSubSystem*> MEngineSubsystemRegistry::subSystems;

void MEngineSubsystemRegistry::init()
{
    MLOG("MEngineSubsystemRegistry:: Initialising registry");
}

void MEngineSubsystemRegistry::cleanup()
{
    MLOG("MEngineSubsystemRegistry:: Cleaning registry");
    int total = subSystems.size();
    int curr = 0;
    for (auto& [type, inst] : subSystems)
    {
        if (inst)
            inst->cleanup();
        delete inst;

        curr++;
        MLOG(SString::format("MEngineSubsystemRegistry:: {0} / {1} cleaned", curr, total));
    }
    MLOG("MEngineSubsystemRegistry:: Registry clean complete");
    subSystems.clear();
}