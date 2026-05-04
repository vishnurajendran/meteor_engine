//
// Created by ssj5v on 20-04-2026.
//

#ifndef ENGINE_STATICS_H
#define ENGINE_STATICS_H
#include "core/object/object.h"
#include "settings/engine_settings.h"

class MEngineStatics : MObject {
public:
    static void init(const SString& inEngineSettingsPath);
    static void saveAll();

// API
public:
    static MEngineSettings& getEngineSettings() { return engineSettings; }

private:
    static void loadEngineSettings(const SString& path);

private:
    static MEngineSettings engineSettings;
    static SString engineSettingsPath;
};



#endif //ENGINE_STATICS_H
