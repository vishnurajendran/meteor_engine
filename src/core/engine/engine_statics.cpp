//
// Created by ssj5v on 20-04-2026.
//
#include "engine_statics.h"

#include "core/utils/fileio.h"
#include "core/utils/logger.h"

// constants
const SString MEngineStatics::engineSettingsFileName = "engine_settings.xml";

// fields
MEngineSettings MEngineStatics::engineSettings;

void MEngineStatics::init()
{
    MLOG("MEngineStatics:: initialising");
    loadEngineSettings();
}

void MEngineStatics::saveAll()
{
    MLOG("MEngineStatics:: saving data");

    // save engine data
    engineSettings.Serialise(engineSettingsFileName);
}


void MEngineStatics::loadEngineSettings()
{
    if (FileIO::hasFile(engineSettingsFileName))
    {
        pugi::xml_document engineSettingsDoc;
        engineSettingsDoc.load_file(engineSettingsFileName.c_str());
        engineSettings.Deserialize(engineSettingsDoc);
    }
}
