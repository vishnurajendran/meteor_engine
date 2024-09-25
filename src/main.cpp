#include "imgui.h"
#include "imgui-SFML.h"
#include "SFML/Graphics/RenderTexture.hpp"

#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/System/Clock.hpp"
#include "SFML/Window/Event.hpp"

#include "core/meteor_core.h"

extern MApplication* getAppInstance();

int main(){
    MObjectPtr appInst = getAppInstance();
    if(appInst == nullptr)
    {
        MERROR(TEXT("Application Creation Error!!!"));
        return 0;
    }

    appInst->initialise();
    while(appInst->isRunning()){
        appInst->run();
    }
    appInst->cleanup();
    return 0;
}
