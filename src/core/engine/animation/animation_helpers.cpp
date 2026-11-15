#include "animation_helpers.h"

#include "animation_subsystem.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/fileio.h"

MAnimationSubsystem* MAnimationHelpers::getSubsystem()
{
    return MEngineSubsystemRegistry::getSubsystem<MAnimationSubsystem>();
}

SString MAnimationHelpers::displayName(const SString& assetPath)
{
    return FileIO::getFileName(assetPath);
}
