//
// Created by ssj5v on 26-09-2024.
//

#pragma once
#ifndef SCENEENTITYTYPEMAP_H
#define SCENEENTITYTYPEMAP_H

#include "sceneentitytypedeserializer.h"

class MSceneEntityTypeMap : public MObject {
    DEFINE_OBJECT_SUBCLASS(MSceneEntityTypeMap)
public:
    static void registerDeserializer(const SString& type, MSceneEntityTypeDeserializer* deserializer);
    static MSceneEntityTypeDeserializer* getDeserializer(const SString& type);
    static size_t getMapSize() { return getMapping().size(); }
private:
    static std::map<SString, MSceneEntityTypeDeserializer*>& getMapping();
};

#endif //SCENEENTITYTYPEMAP_H
