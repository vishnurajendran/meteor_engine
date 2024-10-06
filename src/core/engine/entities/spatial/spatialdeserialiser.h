//
// Created by ssj5v on 27-09-2024.
//

#ifndef SPATIALDESERIALISER_H
#define SPATIALDESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"


class MSpatialDeserializer : MSceneEntityTypeDeserializer {
public:
    virtual MSpatialEntity* deserialize(pugi::xml_node node) override;
private:
    static const bool registered;
};

#endif //SPATIALDESERIALISER_H
