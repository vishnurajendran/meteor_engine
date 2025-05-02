//
// Created by ssj5v on 25-01-2025.
//

#include "ambient_light_entity_deserialiser.h"

#include "ambient_light.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"

bool MAmbientLightEntityDeserialiser::registered = []()
{
    MSceneEntityTypeMap::registerDeserializer("ambient_light",new MAmbientLightEntityDeserialiser());
    return true;
}();

MSpatialEntity* MAmbientLightEntityDeserialiser::deserialize(pugi::xml_node node)
{
    const auto entity = new MAmbientLightEntity();
    parseSpatialData(node, entity);

    const SString LIGHT_ATTRIB_NODE = "ambient_light";
    const SString LIGHT_COLOR_ATTRIB = "color";
    const SString LIGHT_INTENSITY_ATTRIB = "intensity";
    const auto attribNode = node.child(ATTRIB_NODE.c_str());

    float intensity = 0.1f; //default value
    SVector4 color = {1,1,1, 1}; //default value

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

    //set new values.
    entity->setIntensity(intensity);
    entity->setColor(SColor(color.x, color.y, color.z, color.w));

    return entity;
}
