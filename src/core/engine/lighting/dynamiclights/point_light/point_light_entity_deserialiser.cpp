#include "point_light_entity_deserialiser.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"
#include "point_light.h"

bool MPointLightDeserialiser::registered = []() {
    MSceneEntityTypeMap::registerDeserializer("point_light", new MPointLightDeserialiser());
    return true;
}();

MSpatialEntity* MPointLightDeserialiser::deserialize(pugi::xml_node node)
{
    const auto entity = MSpatialEntity::createInstance<MPointLight>("PointLight");
    parseSpatialData(node, entity);

    const auto attrib = node.child(ATTRIB_NODE.c_str());
    if (!attrib) return entity;
    const auto ln = attrib.child("point_light");
    if (!ln) return entity;

    SVector4 color = {1,1,1,1};
    float intensity = 1.0f, range = 10.0f;

    if (const auto n = ln.child("color"))     parseVector4(n.attribute(ATTRIB_VALUE_KEY.c_str()).value(), color);
    if (const auto n = ln.child("intensity")) intensity = std::stof(n.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    if (const auto n = ln.child("range"))     range     = std::stof(n.attribute(ATTRIB_VALUE_KEY.c_str()).value());

    entity->setColor(SColor(color.x, color.y, color.z, color.w));
    entity->setIntensity(intensity);
    entity->setRange(range);

    if (const auto n = ln.child("castsShadow"))
        entity->setCastsShadow(std::string(n.attribute(ATTRIB_VALUE_KEY.c_str()).value()) != "0");
    if (const auto n = ln.child("smoothShadow"))
        entity->setSmoothShadow(std::string(n.attribute(ATTRIB_VALUE_KEY.c_str()).value()) != "0");

    return entity;
}

pugi::xml_node MPointLightDeserialiser::serialise(MSpatialEntity* entity,
                                                   pugi::xml_node parent)
{
    auto* light = dynamic_cast<MPointLight*>(entity);
    pugi::xml_node node   = writeSpatialBase(entity, parent, "point_light");
    pugi::xml_node attrib = node.child(ATTRIB_NODE.c_str());
    pugi::xml_node ln     = attrib.append_child("point_light");
    writeColor(ln, "color",        light->getColor());
    writeFloat(ln, "intensity",    light->getIntensity());
    writeFloat(ln, "range",        light->getRange());
    writeBool (ln, "castsShadow",  light->getCastsShadow());
    writeBool (ln, "smoothShadow", light->getSmoothShadow());
    return node;
}