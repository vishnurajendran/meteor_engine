//
// Created by Vishnu Rajendran on 2024-09-25.
//

#ifndef METEOR_ENGINE_OPENGLDRAWCALL_H
#define METEOR_ENGINE_OPENGLDRAWCALL_H
#include "drawcall.h"

class MOpenGlDrawCall : public MDrawCall {
protected:
    SVector2 resolution;
public:
    MOpenGlDrawCall()=default;
    void draw() override;
    void setTargetResolution(SVector2 resolution);
protected:
    void printOpenGLDrawErrors();
};


#endif //METEOR_ENGINE_OPENGLDRAWCALL_H
