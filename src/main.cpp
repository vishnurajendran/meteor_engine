#include "core/meteor_core.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include "SFML/OpenGL.hpp"



extern MApplication* getAppInstance();

int main(){
    MObjectPtr appInst = getAppInstance();
    if(appInst == nullptr)
    {
        MERROR(STR("Application Creation Error!!!"));
        return 0;
    }

    appInst->initialise();
    MShaderCompiler::initialiseEngine();
    MSceneManager::getSceneManagerInstance()->loadScene(STR("assets/scenes/test_scene.scml"));
    while(appInst->isRunning()){
        appInst->run();
    }
    appInst->cleanup();
    return 0;
}
