//
// Created by Vishnu Rajendran on 2024-09-18.
//
#pragma once

#ifndef METEOR_ENGINE_OBJECT_H
#define METEOR_ENGINE_OBJECT_H

#include "core/utils/sstring.h"

class MObject {
protected:
    SString guid;
    SString name;
public:
    MObject();
    virtual ~MObject() = default;
    SString getGUID();
    SString toString();
    bool equals(MObject* obj);
};


#endif //METEOR_ENGINE_OBJECT_H
