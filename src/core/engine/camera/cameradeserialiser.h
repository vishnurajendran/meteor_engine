//
// Created by ssj5v on 05-10-2024.
//

#ifndef CAMERADESERIALISER_H
#define CAMERADESERIALISER_H
#include "core/engine/entities/spatial/spatialdeserialiser.h"


class MCameraEntityDeserialiser : public MSceneEntityTypeDeserializer {
private:
    static bool registered;
public:
    MSpatialEntity * deserialize(pugi::xml_node node) override;
};



#endif //CAMERADESERIALISER_H
