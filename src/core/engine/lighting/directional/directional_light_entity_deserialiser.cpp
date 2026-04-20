#include "directional_light_entity_deserialiser.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "directional_light.h"

bool MDirectionalLightEntityDeserialiser::registered = []() {
    MSceneEntityTypeMap::registerDeserializer("directional_light", new MDirectionalLightEntityDeserialiser());
    return true;
}();

MSpatialEntity* MDirectionalLightEntityDeserialiser::deserialize(pugi::xml_node node)
{
    const auto entity = new MDirectionalLight();
    parseSpatialData(node, entity);

    entity->setIntensity(1.0f);
    entity->setColor(SColor(1,1,1,1));

    const auto attrib = node.child(ATTRIB_NODE.c_str());
    if (!attrib) return entity;
    const auto ln = attrib.child("directional_light");
    if (!ln) return entity;

    SVector4 color = {1,1,1,1};
    float intensity = 1.0f;

    if (const auto n = ln.child("color"))     parseVector4(n.attribute(ATTRIB_VALUE_KEY.c_str()).value(), color);
    if (const auto n = ln.child("intensity")) intensity = std::stof(n.attribute(ATTRIB_VALUE_KEY.c_str()).value());

    entity->setIntensity(intensity);
    entity->setColor(SColor(color.x, color.y, color.z, color.w));

    auto* mgr = MLightSystemManager::getInstance();
    if (const auto n = ln.child("castsShadow"))
        mgr->directionalShadowEnabled = std::string(n.attribute(ATTRIB_VALUE_KEY.c_str()).value()) != "0";
    if (const auto n = ln.child("smoothShadow"))
        mgr->smoothShadows = std::string(n.attribute(ATTRIB_VALUE_KEY.c_str()).value()) != "0";

    return entity;
}

pugi::xml_node MDirectionalLightEntityDeserialiser::serialise(MSpatialEntity* entity,
                                                               pugi::xml_node parent)
{
    auto* light = dynamic_cast<MDirectionalLight*>(entity);
    auto* mgr   = MLightSystemManager::getInstance();
    pugi::xml_node node   = writeSpatialBase(entity, parent, "directional_light");
    pugi::xml_node attrib = node.child(ATTRIB_NODE.c_str());
    pugi::xml_node ln     = attrib.append_child("directional_light");
    writeColor(ln, "color",        light->getColor());
    writeFloat(ln, "intensity",    light->getIntensity());
    writeBool (ln, "castsShadow",  mgr->directionalShadowEnabled);
    writeBool (ln, "smoothShadow", mgr->smoothShadows);
    return node;
}