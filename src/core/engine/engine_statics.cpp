//
// Created by ssj5v on 20-04-2026.
//

#include "engine_statics.h"

#include <sstream>

#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "pugixml.hpp"

// Static member definitions
MEngineSettings*    MEngineStatics::engineSettings        = nullptr;
SString             MEngineStatics::engineSettingsPath    = SString();
MPhysicsLayersSettings* MEngineStatics::physicsLayersAsset   = nullptr;
SString             MEngineStatics::physicsLayersAssetPath = SString();
MPlayerSettings*    MEngineStatics::playerSettings        = nullptr;
SString             MEngineStatics::playerSettingsPath    = SString();

void MEngineStatics::saveAll()
{
    MLOG("MEngineStatics:: Saving data");

    if (engineSettings && !engineSettingsPath.empty())
        engineSettings->serialiseToFile(engineSettingsPath);

    if (physicsLayersAsset && !physicsLayersAssetPath.empty())
        physicsLayersAsset->serialiseToFile(physicsLayersAssetPath);

    if (playerSettings && !playerSettingsPath.empty())
        playerSettings->serialiseToFile(playerSettingsPath);
}


// ---------------------------------------------------------------------------
// Packaged builds
// ---------------------------------------------------------------------------

namespace
{
bool parseSettingsDocument(const SString& xml, pugi::xml_document& doc)
{
    if (xml.empty())
        return false;
    return doc.load_buffer(xml.c_str(), xml.size()).status == pugi::status_ok && doc.child("root");
}

SString toXml(SerializedClassBase& settings)
{
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("root");
    settings.serialiseToNode(root);

    std::ostringstream oss;
    doc.save(oss);
    return SString(oss.str());
}
} // namespace

bool MEngineStatics::loadPhysicsLayersFromXml(const SString& xml)
{
    pugi::xml_document doc;
    if (!parseSettingsDocument(xml, doc))
    {
        MERROR("MEngineStatics:: embedded physics layers are missing or corrupt - using defaults");
        return false;
    }

    delete physicsLayersAsset;
    physicsLayersAsset     = new MPhysicsLayersSettings();
    physicsLayersAssetPath = SString();   // never saved back to disk
    physicsLayersAsset->deserialiseFromNode(doc.child("root"));
    return true;
}

bool MEngineStatics::applyProjectSettingsFromXml(const SString& xml)
{
    pugi::xml_document doc;
    if (!engineSettings || !parseSettingsDocument(xml, doc))
    {
        MERROR("MEngineStatics:: project settings are missing or corrupt - using defaults");
        return false;
    }

    // Everything in the document applies (so new render / physics settings
    // are picked up automatically), except the window settings.
    const int resX = engineSettings->resX.get();
    const int resY = engineSettings->resY.get();
    const int fps  = engineSettings->fps.get();
    engineSettings->deserialiseFromNode(doc.child("root"));
    engineSettings->resX.set(resX);
    engineSettings->resY.set(resY);
    engineSettings->fps.set(fps);
    return true;
}

void MEngineStatics::loadPlayerSettings(const SString& path)
{
    delete playerSettings;
    playerSettings     = new MPlayerSettings();
    playerSettingsPath = path;

    if (FileIO::hasFile(path))
        playerSettings->deserialiseFromFile(path);
    else
        playerSettings->serialiseToFile(path);   // give players a file to edit

    if (engineSettings)
    {
        engineSettings->resX.set(playerSettings->resX.get());
        engineSettings->resY.set(playerSettings->resY.get());
        engineSettings->fps.set(playerSettings->fps.get());
    }
}

SString MEngineStatics::physicsLayersToXml()
{
    return physicsLayersAsset ? toXml(*physicsLayersAsset) : SString();
}

SString MEngineStatics::engineSettingsToXml()
{
    if (!engineSettings)
        return {};

    // The editor's settings carry editor-only fields (theme, last opened
    // scene, camera); keep only the engine's own by round-tripping through
    // a plain MEngineSettings.
    pugi::xml_document doc;
    if (!parseSettingsDocument(toXml(*engineSettings), doc))
        return {};
    MEngineSettings engineOnly;
    engineOnly.deserialiseFromNode(doc.child("root"));
    return toXml(engineOnly);
}
