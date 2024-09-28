//
// Created by ssj5v on 26-09-2024.
//

#pragma once
#ifndef SCENEENTITYTYPEMAP_H
#define SCENEENTITYTYPEMAP_H

#include <core/meteor_core_minimal.h>
#include "sceneentitytypedeserializer.h"

class MSceneEntityTypeMap : public MObject {
public:
    static void registerDeserializer(const SString& type, MSceneEntityTypeDeserializer* deserializer);
    static MSceneEntityTypeDeserializer* getDeserializer(const SString& type);
    static size_t getMapSize() { return getMapping().size(); }
private:
    static std::map<SString, MSceneEntityTypeDeserializer*>& getMapping();
};

#endif //SCENEENTITYTYPEMAP_H
