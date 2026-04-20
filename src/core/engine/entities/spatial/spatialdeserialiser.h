#pragma once
#ifndef SPATIALDESERIALISER_H
#define SPATIALDESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"

class MSpatialDeserializer : public MSceneEntityTypeDeserializer {
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
    pugi::xml_node  serialise(MSpatialEntity* entity, pugi::xml_node parent) override;
private:
    static const bool registered;
};
#endif