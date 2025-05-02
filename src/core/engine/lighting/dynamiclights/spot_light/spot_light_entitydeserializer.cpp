//
// Created by ssj5v on 30-04-2025.
//

#include "spot_light_entitydeserializer.h"

#include "core/engine/lighting/lighting_system_manager.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"
#include "spot_light.h"

bool MSpotLightEntityDeserializer::registered = []()
{
    MSceneEntityTypeMap::registerDeserializer("spot_light", new MSpotLightEntityDeserializer());
    return true;
}();

MSpatialEntity* MSpotLightEntityDeserializer::deserialize(pugi::xml_node node)
{
    const auto entity = new MSpotLight();
    parseSpatialData(node, entity);

    const SString LIGHT_ATTRIB_NODE = "spot_light";
    const SString LIGHT_COLOR_ATTRIB = "color";
    const SString LIGHT_INTENSITY_ATTRIB = "intensity";
    const SString LIGHT_RANGE_ATTRIB = "range";
    const SString LIGHT_ANGLE_ATTRIB = "angle";

    const auto attribNode = node.child(ATTRIB_NODE.c_str());

    float intensity = 0.1f; //default value
    SVector4 color = {1,1,1, 1}; //default value
    float range = 10.0f;
    float angle = 45.0f;

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

    if (const auto angleNode = lightAttribNode.child(LIGHT_ANGLE_ATTRIB.c_str())) {
        angle = std::stof(angleNode.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    }

    //set new values.
    entity->setIntensity(intensity);
    entity->setColor(SColor(color.x, color.y, color.z, color.w));
    entity->setRange(range);
    entity->setSpotAngle(angle);

    return entity;
}
