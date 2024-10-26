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

    const SString CAMAERA_ATTRIB_NODE = "camera";
    const SString NEARCLIP_ATTRIB = "nearClip";
    const SString FARCLIP_ATTRIB = "farClip";
    const SString PRIORITY_ATTRIB = "priority";
    const SString FOV_ATTRIB = "fov";
    const SString ORTHO_ATTRIB = "orthographic";

    const auto attribNode = node.child(ATTRIB_NODE.c_str());
    if(!attribNode) {
        return entity;
    }

    const auto cameraNode = attribNode.child(CAMAERA_ATTRIB_NODE.c_str());
    if(!cameraNode) {
        return entity;
    }

    auto nearClipVal = 0.1f;
    auto farClipVal = 100.0f;
    auto fov = 60.0f;
    auto priority = 1;
    auto orthographic = false;

    if(const auto ncNode = cameraNode.child(NEARCLIP_ATTRIB.c_str())) {
        nearClipVal = std::stof(ncNode.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    }

    if(const auto fcNode = cameraNode.child(FARCLIP_ATTRIB.c_str())) {
        farClipVal = std::stof(fcNode.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    }

    if(const auto fovNode = cameraNode.child(FOV_ATTRIB.c_str())) {
        fov = std::stof(fovNode.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    }

    if(const auto priorityNode = cameraNode.child(PRIORITY_ATTRIB.c_str())) {
        priority = std::stoi(priorityNode.attribute(ATTRIB_VALUE_KEY.c_str()).value());
    }

    if(const auto orthographicNode = cameraNode.child(ORTHO_ATTRIB.c_str())) {
        orthographic = orthographicNode.attribute(ATTRIB_VALUE_KEY.c_str()).value() == "1";
    }

    entity->setClipPlanes(nearClipVal, farClipVal);
    entity->setFov(fov);
    entity->setPriority(priority);
    entity->setOrthographic(orthographic);

    return entity;
}
