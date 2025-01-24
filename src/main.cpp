#include "imgui.h"
#include "imgui-SFML.h"
#include "SFML/OpenGL.hpp"
#include "core/meteor_core.h"


extern MApplication* getAppInstance();

int main(){
    MObjectPtr appInst = getAppInstance();
    if(appInst == nullptr)
    {
        MERROR(STR("Application Creation Error!!!"));
        return 0;
    }

    appInst->initialise();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    MSceneManager::getSceneManagerInstance()->loadScene(STR("assets/scenes/test_scene.scml"));
    while(appInst->isRunning()){
        appInst->run();
    }
    appInst->cleanup();
    return 0;
}
