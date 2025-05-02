//
// Created by ssj5v on 27-01-2025.
//

#ifndef DIRECTIONAL_LIGHT_ENTITY_DESERIALISER_H
#define DIRECTIONAL_LIGHT_ENTITY_DESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"


class MDirectionalLightEntityDeserialiser : public MSceneEntityTypeDeserializer {
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
private:
    static bool registered;
};



#endif //DIRECTIONAL_LIGHT_ENTITY_DESERIALISER_H
