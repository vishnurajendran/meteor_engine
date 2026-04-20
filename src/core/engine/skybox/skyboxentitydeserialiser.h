#pragma once
#ifndef SKYBOXENTITYDESERIALISER_H
#define SKYBOXENTITYDESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"

class MSkyboxEntityDeserialiser : public MSceneEntityTypeDeserializer {
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
    pugi::xml_node  serialise(MSpatialEntity* entity, pugi::xml_node parent) override;
private:
    static bool registered;
};
#endif