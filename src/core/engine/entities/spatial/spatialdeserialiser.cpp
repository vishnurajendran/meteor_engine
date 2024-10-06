//
// Created by ssj5v on 27-09-2024.
//

#include "spatialdeserialiser.h"

#include "core/engine/scene/serialisation/sceneentitytypemap.h"

const bool MSpatialDeserializer::registered = []() {
    MSceneEntityTypeMap::registerDeserializer("mspatial", new MSpatialDeserializer());
    return true;
}();

MSpatialEntity* MSpatialDeserializer::deserialize(pugi::xml_node node) {
    MSpatialEntity* entity = new MSpatialEntity();
    parseSpatialData(node, entity);
    return entity;
}
