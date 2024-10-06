//
// Created by ssj5v on 26-09-2024.
//

#include "sceneentitytypedeserializer.h"

void MSceneEntityTypeDeserializer::parseSpatialData(pugi::xml_node node, MSpatialEntity *entity) {
    SVector3 localPosition(0);
    SVector3 localScale(1);
    SQuaternion localRotation = glm::identity<SQuaternion>();

    if (node.attribute(NAME_ATTRIB.c_str())) {
        entity->setName(node.attribute(NAME_ATTRIB.c_str()).value());
    }

    if (node.attribute(LOCAL_POS_ATTRIB.c_str())) {
        parseVector3(node.attribute(LOCAL_POS_ATTRIB.c_str()).value(), localPosition);
    }

    if (node.attribute(LOCAL_ROT_ATTRIB.c_str())) {
        parseQuaternion(node.attribute(LOCAL_ROT_ATTRIB.c_str()).value(), localRotation);
    }

    if (node.attribute(LOCAL_SCALE_ATTRIB.c_str())) {
        parseVector3(node.attribute(LOCAL_SCALE_ATTRIB.c_str()).value(), localScale);
    }

    entity->setRelativePosition(localPosition);
    entity->setRelativeRotation(localRotation);
    entity->setRelativeScale(localScale);
}
