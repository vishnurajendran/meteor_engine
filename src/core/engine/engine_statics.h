//
// Created by ssj5v on 20-04-2026.
//

#ifndef ENGINE_STATICS_H
#define ENGINE_STATICS_H
#include "core/object/object.h"
#include "settings/engine_settings.h"

class MEngineStatics : MObject {
private:
    static const SString engineSettingsFileName;

public:
    static void init();
    static void saveAll();
// API
public:
    static MEngineSettings& getEngineSettings() { return engineSettings; }

private:
    static void loadEngineSettings();

private:
    static MEngineSettings engineSettings;
};



#endif //ENGINE_STATICS_H
