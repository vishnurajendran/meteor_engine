#pragma once
#ifndef POINT_LIGHT_DESERIALISER_H
#define POINT_LIGHT_DESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"

class MPointLightDeserialiser : public MSceneEntityTypeDeserializer {
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
    pugi::xml_node  serialise(MSpatialEntity* entity, pugi::xml_node parent) override;
private:
    static bool registered;
};
#endif