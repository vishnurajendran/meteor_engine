//
// Created by ssj5v on 06-10-2024.
//

#pragma once
#ifndef STATICMESHENTITYDESERIALISER_H
#define STATICMESHENTITYDESERIALISER_H
#include "core/engine/scene/serialisation/sceneentitytypedeserializer.h"


class MStaticMeshEntityDeserialiser : MSceneEntityTypeDeserializer {
    static bool registered;
public:
    MSpatialEntity * deserialize(pugi::xml_node node) override;
};



#endif //STATICMESHENTITYDESERIALISER_H
