//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once
#ifndef METEOR_ENGINE_GRAPHICSRENDERER_H
#define METEOR_ENGINE_GRAPHICSRENDERER_H
#include "core/meteor_core_minimal.h"
#include "drawcall.h"

class MOpenGlDrawCall;
class MSFMLDrawCall;

class MGraphicsRenderer : MObject {
private:
    static sf::RenderTarget* renderTarget;
    static std::vector<MSFMLDrawCall*> sfmlDrawCalls;
    static std::vector<MOpenGlDrawCall*> openGlDrawCalls;
public:
    static bool drawGizmos;

    static SVector2 getResolution();
    static void initialise(sf::RenderTarget* target);
    static void submit(MDrawCall* drawCall);
    static void prepare();
    static void draw();
    static void draw(sf::RenderTarget* target);
};


#endif //METEOR_ENGINE_GRAPHICSRENDERER_H
