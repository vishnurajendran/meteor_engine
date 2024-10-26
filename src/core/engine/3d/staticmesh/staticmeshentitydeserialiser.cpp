//
// Created by ssj5v on 06-10-2024.
//

#include "staticmeshentitydeserialiser.h"

#include "staticmeshentity.h"
#include "core/engine/3d/material/MMaterialAsset.h"
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"

bool MStaticMeshEntityDeserialiser::registered = []() {
    MSceneEntityTypeMap::registerDeserializer("mstaticmesh", new MStaticMeshEntityDeserialiser());
    return true;
}();

MSpatialEntity * MStaticMeshEntityDeserialiser::deserialize(pugi::xml_node node) {
    const auto entity = new MStaticMeshEntity();
    parseSpatialData(node, entity);

    const SString ATTRIB_MESH_ATTRIB_PATH = "staticmesh";
    const SString MESH_ATTRIB_SRC = "src";
    const SString MESH_ATTRIB_MATERIAL = "material";
    const auto attribNode = node.child(ATTRIB_NODE.c_str());
    if(!attribNode) {
        return entity;
    }

    const auto meshNode = attribNode.child(ATTRIB_MESH_ATTRIB_PATH.c_str());
    if(!meshNode) {
        return entity;
    }

    if(const auto meshSrcNode = meshNode.child(MESH_ATTRIB_SRC.c_str())) {
        if(const auto meshAsset = MAssetManager::getInstance()->getAsset<MStaticMeshAsset>(meshSrcNode.attribute(ATTRIB_VALUE_KEY.c_str()).value())) {
            entity->setStaticMeshAsset(meshAsset);
        }
        else
            MERROR("Failed to load mesh asset");
    }
    if(const auto meshMatNode = meshNode.child(MESH_ATTRIB_MATERIAL.c_str())) {
        if(const auto material = MAssetManager::getInstance()->getAsset<MMaterialAsset>(meshMatNode.attribute(ATTRIB_VALUE_KEY.c_str()).value())) {
            entity->setMaterial( material->getInstance());
        }
        else
            MERROR("Failed to load material asset");
    }
    return entity;
}
