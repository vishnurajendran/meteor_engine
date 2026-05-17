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
    for (auto& [type, inst] : subSystems)
    {
        inst->cleanup();
        delete inst;
    }

    subSystems.clear();
}