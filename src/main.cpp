#include <GL/glew.h>
#include "core/application/application.h"
#include "core/engine/engine_statics.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/object/objectpointer.h"
#include "core/utils/meteor_paths.h"
#include "data/serialized_class_base.h"
#include "imgui-SFML.h"
#include "imgui.h"

extern MApplication* getAppInstance();

/// Entry point for the application. depending on build mode,
/// Application will load the appropriate instance and run it.
///
/// Arguments:
///   --p <file.mtproj>   project to open (relative to the working directory,
///                       which the launcher sets to the project folder)
int main(int argc, char** argv) {
    // Must run before anything touches the filesystem: it resolves
    // ENGINE_PATH / PROJECT_PATH and makes the project the working directory.
    SMeteorPaths::initialise(argc, argv);

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

    appInst->cleanup();
    MEngineSubsystemRegistry::cleanup();
    MLOG("Application exiting...");
    return 0;
}
