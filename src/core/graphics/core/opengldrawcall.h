//
// Created by Vishnu Rajendran on 2024-09-25.
//

#ifndef METEOR_ENGINE_OPENGLDRAWCALL_H
#define METEOR_ENGINE_OPENGLDRAWCALL_H
#include "drawcall.h"

class MOpenGlDrawCall : public MDrawCall {
public:
    MOpenGlDrawCall()=default;
    void draw() override = 0;
};


#endif //METEOR_ENGINE_OPENGLDRAWCALL_H
