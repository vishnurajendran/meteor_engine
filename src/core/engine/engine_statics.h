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
#include "settings/player_settings.h"

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
        // No path: defaults only, never saved (the player fills these from
        // the project's settings instead).
        if (path.empty())
            return;
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
        // Packaged builds pass no physics path and use the embedded copy
        // instead (loadPhysicsLayersFromXml).
        if (!paths.physicsLayerSettingsPath.empty())
            loadPhysicsLayers(paths.physicsLayerSettingsPath);
    }

    // ---- Packaged builds ---------------------------------------------------
    // Physics settings are baked into the .manifest at publish time. They are
    // loaded from memory and never written back to disk, so players cannot
    // change them.

    // Replaces the physics layers with the embedded document.
    static bool loadPhysicsLayersFromXml(const SString& xml);

    // Applies the project's engine settings (rendering, physics) from an
    // engine settings document onto the active engine settings. Window
    // settings (resolution, fps) in it are ignored; those come from the
    // player settings.
    static bool applyProjectSettingsFromXml(const SString& xml);

    // ---- Player settings -----------------------------------------------------
    // Loads PlayerSettings.xml (writing it with defaults if missing) and
    // applies its window settings to the engine settings. Call after the
    // engine settings are loaded.
    static void loadPlayerSettings(const SString& path);
    static MPlayerSettings* getPlayerSettings() { return playerSettings; }

    // ---- Publishing (editor) -------------------------------------------------
    // The active settings as XML documents, for embedding in a build.
    static SString physicsLayersToXml();
    static SString engineSettingsToXml();


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

    static MPlayerSettings*     playerSettings;
    static SString              playerSettingsPath;
};

#endif // ENGINE_STATICS_H