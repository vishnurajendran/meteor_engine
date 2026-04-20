#include "scene_io.h"
#include "pugixml.hpp"
#include "serialisation/sceneentitytypemap.h"
#include "serialisation/sceneentitytypedeserializer.h"
#include "core/engine/entities/spatial/spatial.h"
#include "scene.h"
#include "scenemanager.h"

SString MSceneIO::s_currentPath = "";

// ── Save ─────────────────────────────────────────────────────────────────────

static void serialiseEntity(MSpatialEntity* entity, pugi::xml_node parentNode)
{
    if (!entity) return;

    const SString typeName = entity->typeName();
    auto* handler = MSceneEntityTypeMap::getDeserializer(typeName);
    if (!handler)
    {
        MWARN(("MSceneIO: no handler for type '" + typeName + "', skipping.").c_str());
        return;
    }

    pugi::xml_node entityNode = handler->serialise(entity, parentNode);
    for (auto* child : entity->getChildren())
        serialiseEntity(child, entityNode);
}

bool MSceneIO::saveScene(const SString& filePath)
{
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("mscene");

    auto* scene = MSceneManager::getSceneManagerInstance()->getActiveScene();

    if (scene == nullptr)
    {
        MERROR("MSceneIO::saveScene: scene is null.");
        return false;
    }

    auto sceneName = scene->getName();
    if (sceneName.empty())
        sceneName = "Untitled";
    root.append_attribute("name") = sceneName.c_str();
    for (const auto& [obj, ptr] : scene->getAllEntities())
    {
        MSpatialEntity* entity = ptr.get();
        if (!entity || entity->getParent() != nullptr) continue;
        serialiseEntity(entity, root);
    }

    if (!doc.save_file(filePath.c_str()))
    {
        MERROR(("MSceneIO::saveScene failed to write: " + filePath).c_str());
        return false;
    }

    s_currentPath = filePath;
    MLOG(("MSceneIO: saved to " + filePath).c_str());
    return true;
}

bool MSceneIO::saveCurrentScene()
{
    if (s_currentPath.empty())
    {
        MWARN("MSceneIO::saveCurrentScene: no path set — open or save a file first.");
        return false;
    }
    return saveScene(s_currentPath);
}

// ── Load ─────────────────────────────────────────────────────────────────────

bool MSceneIO::loadScene(const SString& filePath)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filePath.c_str());
    if (!result)
    {
        MERROR(("MSceneIO::loadScene failed: " + filePath
                + " — " + SString(result.description())).c_str());
        return false;
    }

    // closeActiveScene() → MScene::onClose() destroys all entities and
    // resets subsystems, then loadEmptyScene() creates a fresh MScene.
    auto* mgr = MSceneManager::getSceneManagerInstance();
    mgr->loadEmptyScene();

    auto* scene = mgr->getActiveScene();
    if (scene) scene->tryParse(&doc);

    s_currentPath = filePath;
    MLOG(("MSceneIO: loaded " + filePath).c_str());
    return true;
}

bool MSceneIO::newScene()
{
    // loadEmptyScene closes the current scene (destroying all entities)
    // then creates a fresh empty one.
    MSceneManager::getSceneManagerInstance()->loadEmptyScene();
    s_currentPath = "";
    MLOG("MSceneIO: new scene.");
    return true;
}