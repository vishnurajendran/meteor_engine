//
// Created by Vishnu Rajendran on 2024-09-18.
//
#pragma once

#ifndef METEOR_ENGINE_OBJECT_H
#define METEOR_ENGINE_OBJECT_H

#include "core/utils/sstring.h"

class MObject
{
protected:
    SString guid;
    SString name;

public:
    MObject();
    virtual ~MObject() = default;

    SString getGUID()  const;               // was non-const
    virtual SString toString() const;       // was non-const
    virtual bool    equals(const MObject* obj) const;  // was non-const, non-const param

    void    setName(const SString& newName) { name = newName; }
    SString getName() const                 { return name; }

    virtual MObject* clone() const {};
};

#endif //METEOR_ENGINE_OBJECT_H
