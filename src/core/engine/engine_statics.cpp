//
// Created by ssj5v on 20-04-2026.
//

#include "engine_statics.h"

#include "core/utils/fileio.h"
#include "core/utils/logger.h"

// Static member definitions
MEngineSettings*    MEngineStatics::engineSettings        = nullptr;
SString             MEngineStatics::engineSettingsPath    = SString();
MPhysicsLayersSettings* MEngineStatics::physicsLayersAsset   = nullptr;
SString             MEngineStatics::physicsLayersAssetPath = SString();

void MEngineStatics::saveAll()
{
    MLOG("MEngineStatics:: Saving data");

    if (engineSettings && !engineSettingsPath.empty())
        engineSettings->serialiseToFile(engineSettingsPath);

    if (physicsLayersAsset && !physicsLayersAssetPath.empty())
        physicsLayersAsset->serialiseToFile(physicsLayersAssetPath);
}