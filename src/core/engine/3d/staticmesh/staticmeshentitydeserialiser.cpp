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
    auto entity = new MStaticMeshEntity();
    parseSpatialData(node, entity);

    const SString ATTRIB_MESH_ASSET_PATH = "mesh";
    const SString ATTRIB_MATERIAL_ASSET_PATH = "material";

    if(node.attribute(ATTRIB_MESH_ASSET_PATH.c_str())) {
        auto meshAsset = MAssetManager::getInstance()->getAsset<MStaticMeshAsset>(node.attribute(ATTRIB_MESH_ASSET_PATH.c_str()).value());
        if(meshAsset) {
            entity->setStaticMeshAsset(meshAsset);
        }
        else
            MERROR("Failed to load mesh asset");
    }
    if(node.attribute(ATTRIB_MATERIAL_ASSET_PATH.c_str())) {
        auto material = MAssetManager::getInstance()->getAsset<MMaterialAsset>(node.attribute(ATTRIB_MATERIAL_ASSET_PATH.c_str()).value());
        if(material) {
            entity->setMaterial( material->getInstance());
        }
        else
            MERROR("Failed to load material asset");
    }
    return entity;
}
