//
// Created by ssj5v on 25-01-2025.
//

#ifndef AMBIENT_LIGHT_ENTITY_DESERIALISER_H
#define AMBIENT_LIGHT_ENTITY_DESERIALISER_H
#include "core/engine/entities/spatial/spatialdeserialiser.h"


class MAmbientLightEntityDeserialiser : public MSceneEntityTypeDeserializer {
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
private:
    static bool registered;
};



#endif //AMBIENT_LIGHT_ENTITY_DESERIALISER_H
