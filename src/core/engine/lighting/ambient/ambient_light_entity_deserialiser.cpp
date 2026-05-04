#include "ambient_light_entity_deserialiser.h"
#include "ambient_light.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"

bool MAmbientLightEntityDeserialiser::registered = []() {
    MSceneEntityTypeMap::registerDeserializer("ambient_light", new MAmbientLightEntityDeserialiser());
    return true;
}();

MSpatialEntity* MAmbientLightEntityDeserialiser::deserialize(pugi::xml_node node)
{
    const auto entity = MSpatialEntity::createInstance<MAmbientLightEntity>("AmbientLight");;
    parseSpatialData(node, entity);

    entity->setIntensity(0.1f);
    entity->setColor(SColor(1,1,1,1));

    const auto attrib = node.child(ATTRIB_NODE.c_str());
    if (!attrib) return entity;
    const auto ln = attrib.child("ambient_light");
    if (!ln) return entity;

    SVector4 color = {1,1,1,1};
    float intensity = 0.1f;

    if (const auto n = ln.child("color"))
        parseVector4(n.attribute(ATTRIB_VALUE_KEY.c_str()).value(), color);
    if (const auto n = ln.child("intensity"))
        intensity = std::stof(n.attribute(ATTRIB_VALUE_KEY.c_str()).value());

    entity->setIntensity(intensity);
    entity->setColor(SColor(color.x, color.y, color.z, color.w));
    return entity;
}

pugi::xml_node MAmbientLightEntityDeserialiser::serialise(MSpatialEntity* entity,
                                                           pugi::xml_node parent)
{
    auto* light = dynamic_cast<MAmbientLightEntity*>(entity);
    pugi::xml_node node   = writeSpatialBase(entity, parent, "ambient_light");
    pugi::xml_node attrib = node.child(ATTRIB_NODE.c_str());
    pugi::xml_node ln     = attrib.append_child("ambient_light");
    writeColor(ln, "color",     light->getColor());
    writeFloat(ln, "intensity", light->getIntensity());
    return node;
}