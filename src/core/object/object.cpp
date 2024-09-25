//
// Created by Vishnu Rajendran on 2024-09-18.
//

#include "object.h"
#include "core/utils/guid.h"
#include "core/object/gc.h"

MObject::MObject() {
    guid = SGuid::newGUID();
    name = "MObject";
}

SString MObject::toString() {
    return name+TEXT(" (GUID: ")+guid + TEXT(")");
}

bool MObject::equals(MObject *obj) {
    if(obj != nullptr)
        return obj->guid == guid;
    return false;
}

SString MObject::getGUID() {
    return guid;
}

