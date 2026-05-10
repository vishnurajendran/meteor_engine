#pragma once
#ifndef SPOT_LIGHT_DESERIALISER_H
#define SPOT_LIGHT_DESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"
#include "core/object/object_class_macros.h"

class MSpotLightDeserialiser : public MSceneEntityTypeDeserializer {
    DEFINE_OBJECT_SUBCLASS(MSpotLightDeserialiser)
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
    pugi::xml_node  serialise(MSpatialEntity* entity, pugi::xml_node parent) override;
private:
    static bool registered;
};
#endif