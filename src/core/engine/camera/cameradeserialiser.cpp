#include "cameradeserialiser.h"
#include "camera.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"

bool MCameraEntityDeserialiser::registered = []() {
    MSceneEntityTypeMap::registerDeserializer("camera", new MCameraEntityDeserialiser());
    return true;
}();

MSpatialEntity* MCameraEntityDeserialiser::deserialize(pugi::xml_node node)
{
    auto* entity = new MCameraEntity();
    parseSpatialData(node, entity);

    const auto attrib = node.child(ATTRIB_NODE.c_str());
    if (!attrib) return entity;
    const auto cn = attrib.child("camera");
    if (!cn) return entity;

    float nearClip = 0.1f, farClip = 100.0f, fov = 60.0f;
    int   priority = 1;
    bool  ortho    = false;

    if (const auto n = cn.child("nearClip"))     nearClip = std::stof(n.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    if (const auto n = cn.child("farClip"))      farClip  = std::stof(n.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    if (const auto n = cn.child("fov"))          fov      = std::stof(n.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    if (const auto n = cn.child("priority"))     priority = std::stoi(n.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    if (const auto n = cn.child("orthographic")) ortho    = std::string(n.attribute(ATTRIB_VALUE_KEY.c_str()).value()) != "0";

    entity->setClipPlanes(nearClip, farClip);
    entity->setFov(fov);
    entity->setPriority(priority);
    entity->setOrthographic(ortho);
    return entity;
}

pugi::xml_node MCameraEntityDeserialiser::serialise(MSpatialEntity* entity,
                                                     pugi::xml_node parent)
{
    auto* cam  = dynamic_cast<MCameraEntity*>(entity);
    pugi::xml_node node   = writeSpatialBase(entity, parent, "camera");
    pugi::xml_node attrib = node.child(ATTRIB_NODE.c_str());
    pugi::xml_node cn     = attrib.append_child("camera");
    writeFloat(cn, "nearClip",     cam->getClipPlanes().first);
    writeFloat(cn, "farClip",      cam->getClipPlanes().second);
    writeFloat(cn, "fov",          cam->getFov());
    writeInt  (cn, "priority",     cam->getPriority());
    writeBool (cn, "orthographic", cam->getOrthographic());
    return node;
}