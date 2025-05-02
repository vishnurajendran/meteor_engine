//
// Created by ssj5v on 30-04-2025.
//

#ifndef SPOT_LIGHT_ENTITYDESERIALIZER_H
#define SPOT_LIGHT_ENTITYDESERIALIZER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"


class MSpotLightEntityDeserializer : public MSceneEntityTypeDeserializer {
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
private:
    static bool registered;
};



#endif //SPOT_LIGHT_ENTITYDESERIALIZER_H
