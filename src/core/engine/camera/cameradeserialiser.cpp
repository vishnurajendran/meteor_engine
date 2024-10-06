//
// Created by ssj5v on 05-10-2024.
//

#include "cameradeserialiser.h"
#include "camera.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"

bool MCameraEntityDeserialiser::registered = []() {
    MSceneEntityTypeMap::registerDeserializer("mcamera", new MCameraEntityDeserialiser());
    return true;
}();

MSpatialEntity * MCameraEntityDeserialiser::deserialize(pugi::xml_node node) {
    MCameraEntity* entity = new MCameraEntity();
    parseSpatialData(node, entity);

    const SString NEARCLIP_ATTRIB = "nearClip";
    const SString FARCLIP_ATTRIB = "farClip";
    const SString PRIORITY_ATTRIB = "priority";
    const SString FOV_ATTRIB = "fov";
    const SString ORTHO_ATTRIB = "orthographic";

    auto nearClipVal = std::stof(node.attribute(NEARCLIP_ATTRIB.c_str()).value());
    auto farClipVal = std::stof(node.attribute(FARCLIP_ATTRIB.c_str()).value());
    auto fov = std::stof(node.attribute(FOV_ATTRIB.c_str()).value());
    auto priority = std::stof(node.attribute(PRIORITY_ATTRIB.c_str()).value());
    auto orthographic = node.attribute(PRIORITY_ATTRIB.c_str()).value() == "1";

    entity->setClipPlanes(nearClipVal, farClipVal);
    entity->setFov(fov);
    entity->setPriority(priority);
    entity->setOrthographic(orthographic);

    return entity;
}
