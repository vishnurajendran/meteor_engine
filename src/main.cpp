#include "imgui.h"
#include "imgui-SFML.h"
#include "SFML/OpenGL.hpp"
#include "SFML/Graphics/CircleShape.hpp"

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

    while(appInst->isRunning()){

        sf::CircleShape circleShape;
        circleShape.setRadius(100);
        circleShape.setPosition(100,100);
        circleShape.setFillColor(sf::Color::Blue);

        auto drawCall = new MSFMLDrawCall(&circleShape);
        MGraphicsRenderer::submit(drawCall);
        MGraphicsRenderer::submit(new MOpenGlDrawCall());
        appInst->run();
    }
    appInst->cleanup();
    return 0;
}
