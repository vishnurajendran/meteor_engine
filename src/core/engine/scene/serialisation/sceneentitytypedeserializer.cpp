//
// Created by ssj5v on 26-09-2024.
//

#include "sceneentitytypedeserializer.h"

const SString MSceneEntityTypeDeserializer::ATTRIB_NODE= "attrib";

void MSceneEntityTypeDeserializer::parseSpatialData(pugi::xml_node node, MSpatialEntity *entity) {
    auto attribNode = node.child(ATTRIB_NODE.c_str());
    if(!attribNode) {
        return;
    }

    auto transformNode = attribNode.child(TRANSFORM_NODE.c_str());
    if(!transformNode) {
        return;
    }

    SVector3 localPosition(0);
    SVector3 localScale(1);
    SQuaternion localRotation = glm::identity<SQuaternion>();

    if (node.attribute(NAME_ATTRIB.c_str())) {
        entity->setName(node.attribute(NAME_ATTRIB.c_str()).value());
    }

    if (const auto lpNode = transformNode.child(LOCAL_POS_ATTRIB.c_str())) {
         parseVector3(lpNode.attribute(ATTRIB_VALUE_KEY.c_str()).value(), localPosition);
    }

    if (const auto lrNode = transformNode.child(LOCAL_ROT_ATTRIB.c_str())) {
        parseQuaternion(lrNode.attribute(ATTRIB_VALUE_KEY.c_str()).value(), localRotation);
    }

    if (const auto lsNode = transformNode.child(LOCAL_SCALE_ATTRIB.c_str())) {
        parseVector3(lsNode.attribute(ATTRIB_VALUE_KEY.c_str()).value(), localScale);
    }

    entity->setRelativePosition(localPosition);
    entity->setRelativeRotation(localRotation);
    entity->setRelativeScale(localScale);
}
