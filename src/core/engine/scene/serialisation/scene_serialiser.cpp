#include "scene_serialiser.h"
#include <iostream>
#include <pugixml.hpp>
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"

bool MSceneSerializer::save(MScene* scene, const std::string& filePath)
{
    if (!scene)
    {
       MERROR("SceneSerializer::save() called with null scene");
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

    if (!doc.save_file(filePath.c_str()))
    {
        MERROR(SString::format("SceneSerializer::Failed to write: {0}",filePath));
        return false;
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

    pugi::xml_document    doc;
    pugi::xml_parse_result result = doc.load_file(filePath.c_str());
    if (!result)
    {
        MERROR(SString::format("SceneSerializer::Failed to parse: {0} - {1}", filePath, result.description()));
        return false;
    }

    pugi::xml_node root = doc.child("scene");
    if (!root)
    {
        MERROR(SString::format("SceneSerializer::No <scene> root element in: {0}",filePath));;
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

    MLOG(SString::format("SceneSerializer::Scene loaded from: {0}", filePath));
    return true;
}