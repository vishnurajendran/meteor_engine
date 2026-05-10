#pragma once
#ifndef CAMERADESERIALISER_H
#define CAMERADESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"
#include "core/object/object_class_macros.h"

class MCameraEntityDeserialiser : public MSceneEntityTypeDeserializer {
    DEFINE_OBJECT_SUBCLASS(MCameraEntityDeserialiser)
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
    pugi::xml_node  serialise(MSpatialEntity* entity, pugi::xml_node parent) override;
private:
    static bool registered;
};
#endif