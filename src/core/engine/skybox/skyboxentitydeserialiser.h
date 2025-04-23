//
// Created by ssj5v on 20-04-2025.
//

#ifndef SKYBOXENTITYDESERIALISER_H
#define SKYBOXENTITYDESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"

class MSkyboxEntityDeserialiser : public MSceneEntityTypeDeserializer
{
public:
    MSpatialEntity* deserialize(pugi::xml_node node) override;
private:
    static bool registered;
};



#endif //SKYBOXENTITYDESERIALISER_H
