//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "SFML/OpenGL.hpp"
#include "graphicsrenderer.h"
#include "opengldrawcall.h"
#include "sfmldrawcall.h"

sf::RenderTarget* MGraphicsRenderer::renderTarget;
std::vector<MSFMLDrawCall*> MGraphicsRenderer::sfmlDrawCalls;
std::vector<MOpenGlDrawCall*> MGraphicsRenderer::openGlDrawCalls;

void MGraphicsRenderer::submit(MDrawCall* drawCall) {
    if(!renderTarget) {
        MERROR(STR("Graphics Render Target NULL, Submission omitted"));
        return;
    }

    if(drawCall) {
        if(const auto glDrawCall = dynamic_cast<MOpenGlDrawCall*>(drawCall)) {
            auto resolution = renderTarget->getSize();
            glDrawCall->setTargetResolution({resolution.x, resolution.y});
            openGlDrawCalls.push_back(glDrawCall);
        }
        else
            sfmlDrawCalls.push_back(dynamic_cast<MSFMLDrawCall*>(drawCall));
    }
    else
        MERROR(STR("drawCall NULL, ignored"));
}

void MGraphicsRenderer::initialise(sf::RenderTarget *target) {
    renderTarget = target;
}

void MGraphicsRenderer::draw() {

    if(!renderTarget) {
        MERROR(STR("Graphics Render Target NULL, Draw cancelled"));
        return;
    }

    auto size = renderTarget->getSize();
    renderTarget->setActive(true);
    glViewport(0,0,size.x, size.y);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for(auto drawCall : openGlDrawCalls){
        if(drawCall){
            drawCall->draw();
        }
        else
            MERROR(STR("Draw Call NULL, ignored"));
    }
    openGlDrawCalls.clear();

    renderTarget->pushGLStates();
    for(const auto drawCall : sfmlDrawCalls){
        if(drawCall){
            drawCall->setTarget(renderTarget);
            drawCall->draw();
        }
        else
            MERROR(STR("Draw Call NULL, ignored"));
    }
    renderTarget->popGLStates();
    sfmlDrawCalls.clear();
}