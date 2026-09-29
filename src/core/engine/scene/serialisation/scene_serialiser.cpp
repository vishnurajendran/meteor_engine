#include "scene_serialiser.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <pugixml.hpp>
#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/logger.h"

bool MSceneSerializer::save(MScene* scene, const std::string& filePath)
{
    if (!scene)
    {
       MERROR("SceneSerializer::save() called with null scene");
        return false;
    }

    auto target = MAssetSources::getWritable();
    if (!target)
    {
        MERROR(SString::format("SceneSerializer::Active asset source is read-only, cannot save: {0}", filePath));
        return false;
    }

    pugi::xml_document doc;
    pugi::xml_node     root = doc.append_child("scene");
    root.append_attribute("name") = scene->getName().c_str();

    for (const auto* entity : scene->getRootEntities())
    {
        if (!entity) continue;
        const bool hidden = (entity->getEntityFlags() & EEntityFlags::HideInEditor)
                            == EEntityFlags::HideInEditor;
        if (hidden) continue;
        entity->serialiseEntity(root);
    }

    std::ostringstream oss;
    doc.save(oss);

    const SString path = MAssetPath::normalize(SString(filePath));
    if (!target->writeText(path, SString(oss.str())))
    {
        MERROR(SString::format("SceneSerializer::Failed to write: {0}",filePath));
        return false;
    }

    // Keep the cached MSceneAsset in sync without waiting for the watcher.
    if (auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>())
    {
        if (auto asset = assetManager->getAsset<MSceneAsset>(path))
            asset->requestReload();
    }

    MLOG(SString::format("SceneSerializer::Scene saved to: {0}",filePath));
    return true;
}

bool MSceneSerializer::load(const std::string& filePath, MScene* scene)
{
    if (!scene)
    {
        MERROR("SceneSerializer::load() called with null scene");
        return false;
    }

    std::vector<uint8_t> bytes;
    if (!MAssetSources::getActive()->readBytes(MAssetPath::normalize(SString(filePath)), bytes))
    {
        MERROR(SString::format("SceneSerializer::Failed to read: {0}", filePath));
        return false;
    }

    pugi::xml_document     doc;
    pugi::xml_parse_result result = doc.load_buffer(bytes.data(), bytes.size());
    if (!result)
    {
        MERROR(SString::format("SceneSerializer::Failed to parse: {0} - {1}", filePath, result.description()));
        return false;
    }

    return loadFromDocument(doc, scene, filePath);
}

bool MSceneSerializer::loadFromDocument(const pugi::xml_document& doc, MScene* scene,
                                        const std::string& debugName)
{
    if (!scene)
    {
        MERROR("SceneSerializer::loadFromDocument() called with null scene");
        return false;
    }

    pugi::xml_node root = doc.child("scene");
    if (!root)
    {
        MERROR(SString::format("SceneSerializer::No <scene> root element in: {0}",debugName));
        return false;
    }

    // Restore the scene name that save() wrote into the name attribute.
    if (auto nameAttr = root.attribute("name"); nameAttr)
        scene->setName(nameAttr.value());

    for (pugi::xml_node entityNode : root.children("entity"))
    {
        if (MSpatialEntity* entity = MSpatialEntity::deserialiseEntity(entityNode))
            scene->addToRoot(entity);
    }

    MLOG(SString::format("SceneSerializer::Scene loaded from: {0}", debugName));
    return true;
}