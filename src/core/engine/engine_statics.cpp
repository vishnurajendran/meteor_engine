//
// Created by ssj5v on 20-04-2026.
//
#include "engine_statics.h"

#include "core/utils/fileio.h"
#include "core/utils/logger.h"

// fields
MEngineSettings MEngineStatics::engineSettings;
SString MEngineStatics::engineSettingsPath = SString();


void MEngineStatics::init(const SString& inEngineSettingsPath)
{
    MLOG("MEngineStatics:: Initialising");

    // engine settings
    engineSettingsPath = inEngineSettingsPath;
    loadEngineSettings(engineSettingsPath);
}

void MEngineStatics::saveAll()
{
    MLOG("MEngineStatics:: Saving data");

    // save engine data
    if (!engineSettingsPath.empty())
        engineSettings.serialiseToFile(engineSettingsPath);
}


void MEngineStatics::loadEngineSettings(const SString& path)
{
    if (FileIO::hasFile(path))
    {
        engineSettings.deserialiseFromFile(path);
    }
    else
        MWARN(SString::format("MEngineStatics:: Failed to load engine settings file: {0}", path));
}
