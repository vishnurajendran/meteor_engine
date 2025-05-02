//
// Created by ssj5v on 29-04-2025.
//

#include "point_light_entity_deserialiser.h"

#include "core/engine/scene/serialisation/sceneentitytypemap.h"
#include "point_light.h"

bool MPointLightEntityDeserializer::registered = []()
{
    MSceneEntityTypeMap::registerDeserializer("point_light", new MPointLightEntityDeserializer());
    return true;
}();

MSpatialEntity* MPointLightEntityDeserializer::deserialize(pugi::xml_node node)
{
    const auto entity = new MPointLight();
    parseSpatialData(node, entity);

    const SString LIGHT_ATTRIB_NODE = "point_light";
    const SString LIGHT_COLOR_ATTRIB = "color";
    const SString LIGHT_INTENSITY_ATTRIB = "intensity";
    const SString LIGHT_RANGE_ATTRIB = "range";

    const auto attribNode = node.child(ATTRIB_NODE.c_str());

    float intensity = 0.1f; //default value
    SVector4 color = {1,1,1, 1}; //default value
    float range = 10.0f;

    //set defaults
    entity->setIntensity(intensity);
    entity->setColor(SColor(color.x, color.y, color.z, color.w));

    const auto lightAttribNode = attribNode.child(LIGHT_ATTRIB_NODE.c_str());
    if(!lightAttribNode) {
        return entity;
    }

    if (const auto colorNode = lightAttribNode.child(LIGHT_COLOR_ATTRIB.c_str())) {
        parseVector4(colorNode.attribute(ATTRIB_VALUE_KEY.c_str()).value(), color);
    }

    if (const auto intensityNode = lightAttribNode.child(LIGHT_INTENSITY_ATTRIB.c_str())) {
        intensity = std::stof(intensityNode.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    }

    if (const auto rangeNode = lightAttribNode.child(LIGHT_RANGE_ATTRIB.c_str())) {
        range = std::stof(rangeNode.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    }

    //set new values.
    entity->setIntensity(intensity);
    entity->setColor(SColor(color.x, color.y, color.z, color.w));
    entity->setRange(range);

    return entity;
}
