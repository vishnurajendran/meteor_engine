//
// Created by ssj5v on 20-04-2026.
//
#include "engine_statics.h"

#include "core/utils/fileio.h"
#include "core/utils/logger.h"

// static field inits
MEngineSettings* MEngineStatics::engineSettings = nullptr;
SString MEngineStatics::engineSettingsPath = SString();


void MEngineStatics::saveAll()
{
    MLOG("MEngineStatics:: Saving data");
    // save engine data
    if (!engineSettingsPath.empty())
        engineSettings->serialiseToFile(engineSettingsPath);
}