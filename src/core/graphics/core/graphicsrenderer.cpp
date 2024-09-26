//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "SFML/OpenGL.hpp"
#include "graphicsrenderer.h"
#include "opengldrawcall.h"

sf::RenderTarget* MGraphicsRenderer::renderTarget;
std::vector<MDrawCall*> MGraphicsRenderer::sfmlDrawCalls;
std::vector<MDrawCall*> MGraphicsRenderer::openGlDrawCalls;

void MGraphicsRenderer::submit(MDrawCall* drawCall) {
    if(!renderTarget) {
        MERROR(TEXT("Graphics Render Target NULL, Submission omitted"));
        return;
    }

    if(drawCall) {
        auto glDrawCall = dynamic_cast<MOpenGlDrawCall*>(drawCall);
        if(glDrawCall){
            openGlDrawCalls.push_back(drawCall);
        }
        else
            sfmlDrawCalls.push_back(drawCall);
    }
    else
        MERROR(TEXT("drawCall NULL, ignored"));
}

void MGraphicsRenderer::initialise(sf::RenderTarget *target) {
    renderTarget = target;
}

void MGraphicsRenderer::draw() {

    if(!renderTarget) {
        MERROR(TEXT("Graphics Render Target NULL, Draw cancelled"));
        return;
    }

    auto size = renderTarget->getSize();
    glViewport(0,0,size.x, size.y);
    renderTarget->clear(sf::Color::Black);
    for(auto drawCall : openGlDrawCalls){
        if(drawCall){
            drawCall->draw(renderTarget);
        }
        else
            MERROR(TEXT("Draw Call NULL, ignored"));
    }

    renderTarget->pushGLStates();
    for(auto drawCall : sfmlDrawCalls){
        if(drawCall){
            drawCall->draw(renderTarget);
        }
        else
            MERROR(TEXT("Draw Call NULL, ignored"));
    }
    renderTarget->popGLStates();
}
