#include <GL/glew.h>
#include "SFML/OpenGL.hpp"
#include "core/engine/engine_statics.h"
#include "core/meteor_core.h"
#include "data/serialized_class_base.h"
#include "imgui-SFML.h"
#include "imgui.h"

extern MApplication* getAppInstance();

int main(){
    MObjectPtr appInst = getAppInstance();
    if(appInst == nullptr)
    {
        MERROR(STR("Application Creation Error!!!"));
        return 0;
    }

    MEngineStatics::init(appInst->getEngineSettingsPath());
    appInst->initialise();

    while(appInst->isRunning()){
        appInst->run();
    }

    MEngineStatics::saveAll();
    appInst->cleanup();
    return 0;
}
