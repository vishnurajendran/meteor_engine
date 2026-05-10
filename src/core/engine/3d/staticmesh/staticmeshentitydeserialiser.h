#pragma once
#ifndef STATICMESHENTITYDESERIALISER_H
#define STATICMESHENTITYDESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"
#include "core/object/object_class_macros.h"

class MStaticMeshEntityDeserialiser : public MSceneEntityTypeDeserializer {
    DEFINE_OBJECT_SUBCLASS(MSpatialEntity)
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
    pugi::xml_node  serialise(MSpatialEntity* entity, pugi::xml_node parent) override;
private:
    static bool registered;
};
#endif