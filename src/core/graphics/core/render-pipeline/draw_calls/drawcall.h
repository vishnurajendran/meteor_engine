//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once

#ifndef METEOR_ENGINE_DRAWCALL_H
#define METEOR_ENGINE_DRAWCALL_H
#include "SFML/Graphics.hpp"
#include "core/object/object.h"

class MDrawCall : public MObject {
    DEFINE_OBJECT_SUBCLASS(MDrawCall)
public:
    MDrawCall() = default;
    virtual void draw()=0;
    virtual int getSortinOrder() {return 0;}
};


#endif //METEOR_ENGINE_DRAWCALL_H
