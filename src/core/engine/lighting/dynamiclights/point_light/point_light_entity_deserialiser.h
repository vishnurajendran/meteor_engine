//
// Created by ssj5v on 29-04-2025.
//

#ifndef POINT_LIGHT_ENTITY_DESERIALISER_H
#define POINT_LIGHT_ENTITY_DESERIALISER_H
#include "core/engine/entities/spatial/spatialdeserialiser.h"


class MPointLightEntityDeserializer : public MSceneEntityTypeDeserializer {
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
private:
    static bool registered;
};



#endif //POINT_LIGHT_ENTITY_DESERIALISER_H
