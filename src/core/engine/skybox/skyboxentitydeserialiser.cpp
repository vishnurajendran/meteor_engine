#include "skyboxentitydeserialiser.h"
#include "skybox.h"
#include "cubemapasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/scene/serialisation/sceneentitytypemap.h"

bool MSkyboxEntityDeserialiser::registered = []() {
    MSceneEntityTypeMap::registerDeserializer("skybox", new MSkyboxEntityDeserialiser());
    return true;
}();

MSpatialEntity* MSkyboxEntityDeserialiser::deserialize(pugi::xml_node node)
{
    const auto entity = MSpatialEntity::createInstance<MSkyboxEntity>("Skybox");
    parseSpatialData(node, entity);

    const auto attrib = node.child(ATTRIB_NODE.c_str());
    if (!attrib) return entity;
    const auto sn = attrib.child("skybox");
    if (!sn) return entity;

    if (const auto n = sn.child("src")) {
        if (const auto cube = MAssetManager::getInstance()->getAsset<MCubemapAsset>(
                n.attribute(ATTRIB_VALUE_KEY.c_str()).value()))
            entity->setCubemapAsset(cube);
        else MERROR("Failed to load cubemap asset");
    }
    return entity;
}

pugi::xml_node MSkyboxEntityDeserialiser::serialise(MSpatialEntity* entity,
                                                     pugi::xml_node parent)
{
    auto* skybox = dynamic_cast<MSkyboxEntity*>(entity);
    pugi::xml_node node   = writeSpatialBase(entity, parent, "skybox");
    pugi::xml_node attrib = node.child(ATTRIB_NODE.c_str());
    pugi::xml_node sn     = attrib.append_child("skybox");
    if (skybox->getCubemapAsset())
        writeString(sn, "src", skybox->getCubemapAsset()->getPath());
    return node;
}