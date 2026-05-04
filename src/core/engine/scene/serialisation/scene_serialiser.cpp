#include "scene_serialiser.h"
#include <iostream>
#include <pugixml.hpp>
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"

// ─── Save ─────────────────────────────────────────────────────────────────────

bool MSceneSerializer::save(MScene* scene, const std::string& filePath)
{
    if (!scene)
    {
        std::cerr << "[SceneSerializer] save() called with null scene\n";
        return false;
    }

    pugi::xml_document doc;
    pugi::xml_node     root = doc.append_child("scene");
    root.append_attribute("name") = scene->getName().c_str();

    // Iterate only ROOT entities — children are written recursively inside
    // each entity's <children> block by serialiseEntity().
    // Skip editor-only entities (e.g. the editor scene camera).
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
        std::cerr << "[SceneSerializer] Failed to write: " << filePath << "\n";
        return false;
    }

    std::cout << "[SceneSerializer] Scene saved to: " << filePath << "\n";
    return true;
}

bool MSceneSerializer::load(const std::string& filePath, MScene* scene)
{
    if (!scene)
    {
        std::cerr << "[SceneSerializer] load() called with null scene\n";
        return false;
    }

    pugi::xml_document    doc;
    pugi::xml_parse_result result = doc.load_file(filePath.c_str());
    if (!result)
    {
        std::cerr << "[SceneSerializer] Failed to parse: " << filePath
                  << " — " << result.description() << "\n";
        return false;
    }

    pugi::xml_node root = doc.child("scene");
    if (!root)
    {
        std::cerr << "[SceneSerializer] No <scene> root element in: " << filePath << "\n";
        return false;
    }

    // Deserialise each top-level <entity> node.
    // MSpatialEntity::deserialiseEntity handles type dispatch, field loading,
    // and recursive child restoration.
    for (pugi::xml_node entityNode : root.children("entity"))
    {
        MSpatialEntity* entity = MSpatialEntity::deserialiseEntity(entityNode);
        if (entity)
            scene->addToRoot(entity);
    }

    std::cout << "[SceneSerializer] Scene loaded from: " << filePath << "\n";
    return true;
}