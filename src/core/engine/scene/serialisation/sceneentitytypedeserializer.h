//
// Created by ssj5v on 26-09-2024.
//

#pragma once
#ifndef SCENEENTITYTYPEDESERIALIZER_H
#define SCENEENTITYTYPEDESERIALIZER_H

#include "core/meteor_core_minimal.h"
#include <core/engine/entities/spatial/spatial.h>
#include <pugixml.hpp>

class MSceneEntityTypeDeserializer : public MObject {
public:
    MSceneEntityTypeDeserializer() {};
    virtual MSpatialEntity* deserialize(pugi::xml_node* node) = 0;
protected:
    void parseSpatialData(pugi::xml_node* node, MSpatialEntity* entity);
private:
    const SString LOCAL_POS_ATTRIB = "localPosition";
    const SString LOCAL_ROT_ATTRIB = "localRotation";
    const SString LOCAL_SCALE_ATTRIB = "localScale";
    const SString NAME_ATTRIB = "name";
};

#endif //SCENEENTITYTYPEDESERIALIZER_H
