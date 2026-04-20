#pragma once
#ifndef DIRECTIONAL_LIGHT_ENTITY_DESERIALISER_H
#define DIRECTIONAL_LIGHT_ENTITY_DESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"

class MDirectionalLightEntityDeserialiser : public MSceneEntityTypeDeserializer {
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
    pugi::xml_node  serialise(MSpatialEntity* entity, pugi::xml_node parent) override;
private:
    static bool registered;
};
#endif