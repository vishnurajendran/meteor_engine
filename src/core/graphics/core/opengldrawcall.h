//
// Created by Vishnu Rajendran on 2024-09-25.
//

#ifndef METEOR_ENGINE_OPENGLDRAWCALL_H
#define METEOR_ENGINE_OPENGLDRAWCALL_H
#include "core/meteor_core_minimal.h"
#include "drawcall.h"
class MOpenGlDrawCall : public MDrawCall {
public:
    MOpenGlDrawCall()=default;
    void draw(sf::RenderTarget *target) override;
};


#endif //METEOR_ENGINE_OPENGLDRAWCALL_H
