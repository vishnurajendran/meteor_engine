#include <GL/glew.h>
#include "SFML/OpenGL.hpp"
#include "core/engine/audio/impl_miniaudio/audio_engine.h"
#include "core/engine/audio/impl_miniaudio/audio_listener.h"
#include "core/engine/audio/impl_miniaudio/audio_source.h"
#include "core/engine/audio/interfaces/engine_interface.h"
#include "core/engine/engine_statics.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/meteor_core.h"
#include "data/serialized_class_base.h"
#include "imgui-SFML.h"
#include "imgui.h"

extern MApplication* getAppInstance();

/// Entry point for the application. depending on build mode,
/// Application will load the appropriate instance and run it.
int main() {
    MObjectPtr appInst = getAppInstance();
    if(appInst == nullptr)
    {
        MERROR(STR("Application Creation Error!!!"));
        return 0;
    }

    MEngineSubsystemRegistry::init();
    appInst->initialise();

    while(appInst->isRunning()){
        appInst->run();
    }

    MEngineStatics::saveAll();
    MEngineSubsystemRegistry::cleanup();
    appInst->cleanup();
    return 0;
}
