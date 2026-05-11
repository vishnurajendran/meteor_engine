//
// Created by ssj5v on 20-04-2026.
//

#ifndef ENGINE_STATICS_H
#define ENGINE_STATICS_H
#include "core/object/object.h"
#include "core/utils/fileio.h"
#include "settings/engine_settings.h"

class MEngineStatics : public MObject {
    DEFINE_OBJECT_SUBCLASS(MEngineStatics)
public:
    static void saveAll();

// API
public:
    static MEngineSettings* getEngineSettings() { return engineSettings; }

    template<typename T>
    static void loadEngineSettings(const SString& path)
    {
        static_assert(std::is_base_of<MEngineSettings, T>::value,
                      "T must derive from MEngineSettings");

        // delete previous settings.
        if (engineSettings)
            delete engineSettings;

        engineSettingsPath = path;
        engineSettings = new T();
        if (FileIO::hasFile(path))
        {
            engineSettings->deserialiseFromFile(path);
        }
        else
            MWARN(SString::format("MEngineStatics:: Failed to load engine settings file: {0}", path));
    }

private:
    static MEngineSettings* engineSettings;
    static SString engineSettingsPath;
};



#endif //ENGINE_STATICS_H
