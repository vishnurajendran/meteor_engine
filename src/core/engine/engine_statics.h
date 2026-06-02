//
// Created by ssj5v on 20-04-2026.
//

#ifndef ENGINE_STATICS_H
#define ENGINE_STATICS_H

#include "core/object/object.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "settings/engine_settings.h"
#include "settings/physics_layers_settings.h"

struct SSettingsPaths
{
    SString engineSettingsPath;
    SString physicsLayerSettingsPath;
};

class MEngineStatics : public MObject {
    DEFINE_OBJECT_SUBCLASS(MEngineStatics)

private:
    // ---- Engine settings ---------------------------------------------------

    template<typename T>
    static void loadEngineSettings(const SString& path)
    {
        static_assert(std::is_base_of<MEngineSettings, T>::value,
                      "T must derive from MEngineSettings");

        if (engineSettings)
            delete engineSettings;

        engineSettingsPath = path;
        engineSettings     = new T();
        if (FileIO::hasFile(path))
            engineSettings->deserialiseFromFile(path);
        else
            MWARN(SString::format("MEngineStatics:: Failed to load engine settings file: {0}", path));
    }

    // Loads (or reloads) the physics layers asset from `path`. Creates a fresh
    // asset with default names if the file does not exist yet - the asset is
    // still available immediately so the rest of the engine can proceed.
    static void loadPhysicsLayers(const SString& path)
    {
        if (physicsLayersAsset)
            delete physicsLayersAsset;

        physicsLayersAssetPath = path;
        physicsLayersAsset     = new MPhysicsLayersSettings();

        if (FileIO::hasFile(path))
            physicsLayersAsset->deserialiseFromFile(path);
        else
            MWARN(SString::format(
                "MEngineStatics:: Physics layers file not found at '{0}' - using defaults. "
                "The file will be created on next save.", path));
    }

    // public API Access for load and saves.
public:

    template<typename T>
    static void loadSettings(const SSettingsPaths& paths)
    {
        loadEngineSettings<T>(paths.engineSettingsPath);
        loadPhysicsLayers(paths.physicsLayerSettingsPath);
    }


    static void saveAll();
    // Returns the active engine settings, or nullptr if one has not been
    // loaded yet.
    static MEngineSettings* getEngineSettings() { return engineSettings; }

    // Returns the active physics layers settings, or nullptr if one has not been
    // loaded yet.
    static MPhysicsLayersSettings* getPhysicsLayers() { return physicsLayersAsset; }

private:
    static MEngineSettings*    engineSettings;
    static SString             engineSettingsPath;

    static MPhysicsLayersSettings* physicsLayersAsset;
    static SString              physicsLayersAssetPath;
};

#endif // ENGINE_STATICS_H