#pragma once
#ifndef AMBIENT_LIGHT_ENTITY_DESERIALISER_H
#define AMBIENT_LIGHT_ENTITY_DESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"
#include "core/object/object_class_macros.h"

class MAmbientLightEntityDeserialiser : public MSceneEntityTypeDeserializer {
    DEFINE_OBJECT_SUBCLASS(MAmbientLightEntityDeserialiser)
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
    pugi::xml_node  serialise(MSpatialEntity* entity, pugi::xml_node parent) override;
private:
    static bool registered;
};
#endif