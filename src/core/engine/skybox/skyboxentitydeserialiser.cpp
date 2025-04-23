//
// Created by ssj5v on 20-04-2025.
//

#include "skyboxentitydeserialiser.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"
#include "cubemapasset.h"
#include "skybox.h"

bool MSkyboxEntityDeserialiser::registered = []()
{
    MSceneEntityTypeMap::registerDeserializer("skybox", new MSkyboxEntityDeserialiser());
    return true;
}();

MSpatialEntity* MSkyboxEntityDeserialiser::deserialize(pugi::xml_node node)
{
    const auto entity = new MSkyboxEntity();
    parseSpatialData(node, entity);

    const SString ATTRIB_SKYBOX_ATTRIB_PATH = "skybox";
    const SString CUBEMAP_ATTRIB_SRC = "src";
    const auto attribNode = node.child(ATTRIB_NODE.c_str());
    if(!attribNode) {
        return entity;
    }

    const auto skyboxNode = attribNode.child(ATTRIB_SKYBOX_ATTRIB_PATH.c_str());
    if(!skyboxNode) {
        return entity;
    }

    if(const auto meshSrcNode = skyboxNode.child(CUBEMAP_ATTRIB_SRC.c_str())) {
        if(const auto cubeMap = MAssetManager::getInstance()->getAsset<MCubemapAsset>(meshSrcNode.attribute(ATTRIB_VALUE_KEY.c_str()).value())) {
            entity->setCubemapAsset(cubeMap);
        }
        else
            MERROR("Failed to Cubemap asset");
    }
    return entity;
}
