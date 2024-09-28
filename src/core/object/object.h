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
    virtual SString toString();
    void setName(const SString& name) { this->name = name; }
    SString getName() const { return name;}
    virtual bool equals(MObject* obj);
};

#endif //METEOR_ENGINE_OBJECT_H
