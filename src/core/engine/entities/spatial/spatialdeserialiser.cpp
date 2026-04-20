#include "spatialdeserialiser.h"

#include "core/engine/scene/serialisation/sceneentitytypemap.h"
#include "spatial.h"

const bool MSpatialDeserializer::registered = []() {
    MSceneEntityTypeMap::registerDeserializer("spatial", new MSpatialDeserializer());
    return true;
}();

MSpatialEntity* MSpatialDeserializer::deserialize(pugi::xml_node node)
{
    MSpatialEntity* entity = MSpatialEntity::createInstance();
    parseSpatialData(node, entity);
    return entity;
}

pugi::xml_node MSpatialDeserializer::serialise(MSpatialEntity* entity,
                                                pugi::xml_node parent)
{
    return writeSpatialBase(entity, parent, "spatial");
}