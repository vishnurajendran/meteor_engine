#include "staticmeshentitydeserialiser.h"
#include "staticmeshentity.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"

bool MStaticMeshEntityDeserialiser::registered = []() {
    MSceneEntityTypeMap::registerDeserializer("static_mesh", new MStaticMeshEntityDeserialiser());
    return true;
}();

MSpatialEntity* MStaticMeshEntityDeserialiser::deserialize(pugi::xml_node node)
{
    const auto entity = MSpatialEntity::createInstance<MStaticMeshEntity>("StaticMeshEntity");
    parseSpatialData(node, entity);

    const auto attrib = node.child(ATTRIB_NODE.c_str());
    if (!attrib) return entity;
    const auto meshNode = attrib.child("static_mesh");
    if (!meshNode) return entity;

    if (const auto n = meshNode.child("src")) {
        if (const auto asset = MAssetManager::getInstance()->getAsset<MStaticMeshAsset>(
                n.attribute(ATTRIB_VALUE_KEY.c_str()).value()))
            entity->setStaticMeshAsset(asset);
        else MERROR("Failed to load mesh asset");
    }
    if (const auto n = meshNode.child("material")) {
        if (const auto asset = MAssetManager::getInstance()->getAsset<MMaterialAsset>(
                n.attribute(ATTRIB_VALUE_KEY.c_str()).value()))
            entity->setMaterialAsset(asset);
        else MERROR("Failed to load material asset");
    }
    if (const auto n = node.child("castsShadow"))
        entity->setCastsShadow(std::string(n.attribute(ATTRIB_VALUE_KEY.c_str()).value()) != "0");

    return entity;
}

pugi::xml_node MStaticMeshEntityDeserialiser::serialise(MSpatialEntity* entity,
                                                         pugi::xml_node parent)
{
    auto* mesh = dynamic_cast<MStaticMeshEntity*>(entity);
    pugi::xml_node node     = writeSpatialBase(entity, parent, "static_mesh");
    pugi::xml_node attrib   = node.child(ATTRIB_NODE.c_str());
    pugi::xml_node meshNode = attrib.append_child("static_mesh");

    if (mesh->getStaticMeshAsset())
        writeString(meshNode, "src", mesh->getStaticMeshAsset()->getPath());
    if (mesh->getMaterialAsset())
        writeString(meshNode, "material", mesh->getMaterialAsset()->getPath());

    writeBool(attrib, "castsShadow", mesh->getCastsShadow());
    return node;
}