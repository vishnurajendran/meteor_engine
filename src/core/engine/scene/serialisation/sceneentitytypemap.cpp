//
// Created by ssj5v on 26-09-2024.
//

#include "sceneentitytypemap.h"

std::map<SString, MSceneEntityTypeDeserializer*>& MSceneEntityTypeMap::getMapping() {
    static std::map<SString, MSceneEntityTypeDeserializer*> mapping;
    return mapping;
}

void MSceneEntityTypeMap::registerDeserializer(const SString& type, MSceneEntityTypeDeserializer* deserializer) {
    auto& mapping = getMapping();
    mapping[type] = deserializer;
}

MSceneEntityTypeDeserializer* MSceneEntityTypeMap::getDeserializer(const SString& type) {
    auto& mapping = getMapping();
    if (mapping.contains(type)) {
        return mapping[type];
    }
    return NULL;
}