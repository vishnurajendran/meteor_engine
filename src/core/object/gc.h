//
// Created by Vishnu Rajendran on 2024-09-18.
//

#pragma once

#ifndef METEOR_ENGINE_GC_H
#define METEOR_ENGINE_GC_H
#include <map>
#include "core/utils/sstring.h"


class MObject;

class MGarbageCollector {
private:
    static std::map<SString, int> objReferenceMap;
public:
    static void reference(MObject* obj);
    static void dereference(MObject* obj);
};


#endif //METEOR_ENGINE_GC_H
