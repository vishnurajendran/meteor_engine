//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once
#ifndef METEOR_ENGINE_GRAPHICSRENDERER_H
#define METEOR_ENGINE_GRAPHICSRENDERER_H
#include "SFML/Graphics.hpp"
#include "core/meteor_core_minimal.h"
#include "drawcall.h"

class MGraphicsRenderer : MObject {
private:
    static sf::RenderTarget* renderTarget;
    static std::vector<MDrawCall*> sfmlDrawCalls;
    static std::vector<MDrawCall*> openGlDrawCalls;
public:
    static void initialise(sf::RenderTarget* target);
    static void submit(MDrawCall* drawCall);
    static void draw();
};


#endif //METEOR_ENGINE_GRAPHICSRENDERER_H
