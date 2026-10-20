//
// Created by ssj5v on 26-09-2024.
//

#include "scene.h"

#include "core/engine/entities/spatial/spatial.h"
#include "serialisation/sceneentitytypemap.h"

const SString MScene::VALID_SCENE_FILE_XML_TAG = "mscene";
const SString MScene::SCENE_NAME_TAG = "name";


MScene::MScene() {
    name = "Untitled";
    startScene();
}

MScene::~MScene() {
    rootEntities.clear();
}

void MScene::startScene()
{
    // Every entity is built and its fields are set - give the engine parts
    // (physics bodies...) a chance to exist before the first onStart runs.
    awakePending();
}

void MScene::awakePending()
{
    // Index loop - an onAwake that spawns entities appends to the list, and
    // those are awoken in this same call.
    for (size_t i = 0; i < pendingAwake.size(); ++i)
    {
        MSpatialEntity* entity = pendingAwake[i];
        if (entity && !entity->hasAwoken() && !markedForDestroy.contains(entity))
            entity->onAwake();
    }
    pendingAwake.clear();
}

void MScene::update(float deltaTime)
{
    destroyMarked();
    awakePending();   // spawned since the last pass - before any onStart below

    for (auto* ptr : getEntityOrder())
    {
        if (ptr == nullptr)
            continue;

        if (!ptr->isEnabledInHierarchy())
            continue;

        // call start if the entity was not started.
        if (!ptr->hasStarted())
            ptr->onStart();

        if (ptr->getCanTick())
            ptr->onUpdate(deltaTime);
    }
}

void MScene::fixedUpdate(float fixedDeltaTime)
{
    awakePending();   // fixedUpdate can start entities too
    for (auto* ptr : getEntityOrder())
    {
        if (ptr == nullptr)
            continue;

        if (!ptr->isEnabledInHierarchy())
            continue;

        // call start if the entity was not started.
        if (!ptr->hasStarted())
            ptr->onStart();

        if (ptr->getCanTick())
            ptr->onFixedUpdate(fixedDeltaTime);
        else
            MLOG(SString::format("NON TICK: {0}", ptr->getName()));
    }
}

void MScene::lateUpdate(float deltaTime)
{
    for (auto* ptr : getEntityOrder())
    {
        if (ptr == nullptr)
            continue;

        if (!ptr->isEnabledInHierarchy())
            continue;

        // update() starts entities - one spawned during this frame's onTick
        // waits for the next frame rather than late-ticking before onStart.
        if (!ptr->hasStarted())
            continue;

        if (ptr->getCanTick())
            ptr->onLateUpdate(deltaTime);
    }
}

void MScene::postPhysicsStep(float fixedDeltaTime)
{
    for (auto* ptr : getEntityOrder())
    {
        if (ptr == nullptr || !ptr->hasStarted() || !ptr->isEnabledInHierarchy())
            continue;

        ptr->onPhysicsStepped(fixedDeltaTime);
    }
}

void MScene::interpolatePhysics(float alpha)
{
    for (auto* ptr : getEntityOrder())
    {
        if (ptr == nullptr || !ptr->hasStarted() || !ptr->isEnabledInHierarchy())
            continue;

        ptr->onPhysicsInterpolate(alpha);
    }
}

void MScene::close()
{
    MLOG(SString::format("Initiating scene closure...{0}", name));
    sceneClosing = true;

    // Walk the roots in hierarchy order; destroy() takes each subtree
    // children-first. Copy - a script's onStop could spawn or reparent.
    const std::vector<MSpatialEntity*> roots = rootEntities;
    for (auto* root : roots)
        if (root)
            root->destroy();

    // Entities not reachable from the roots (e.g. the editor camera, which
    // takes itself out of the root list) still need to go. destroy() skips
    // anything already marked.
    std::vector<MSpatialEntity*> unreached;
    for (const auto& [obj, ptr] : allAliveEntities)
        if (ptr.get() && !markedForDestroy.contains(ptr.get()))
            unreached.push_back(ptr.get());
    for (auto* entity : unreached)
        entity->destroy();

    destroyMarked();
    rootEntities.clear();
    entityOrder.clear();
    entityOrderDirty = true;
    MLOG(STR("Scene closed"));
}

void MScene::markForDestroy(MSpatialEntity* entity)
{
    if (entity == nullptr) return;
    if (markedForDestroy.contains(entity)) return;
    markedForDestroy.insert(entity);
}

void MScene::destroyMarked()
{
    if (markedForDestroy.empty()) return;
    MLOG(SString::format("Marked for destroy {0}", std::to_string(markedForDestroy.size())));
    // Two passes, so the result does not depend on the set's order. Freeing a
    // parent before its child used to leave the child reading (and writing
    // into) the freed parent.
    const std::vector<MSpatialEntity*> batch(markedForDestroy.begin(), markedForDestroy.end());

    // Pass 1 - unlink
    for (auto* entity : batch)
    {
        // Top of a destroyed subtree - detach it from the parent that stays.
        auto* myParent = entity->getParent();
        if (myParent && !markedForDestroy.contains(myParent))
            myParent->removeChild(entity);

        // A child that is not being destroyed (parented after destroy() ran)
        // moves to the scene root rather than keep a freed parent.
        // Copy - removeChild edits the list.
        const std::vector<MSpatialEntity*> children = entity->getChildren();
        for (auto* child : children)
            if (child && !markedForDestroy.contains(child))
                entity->removeChild(child);
    }

    // Pass 2 - free. Marked entities now only point at each other, and no
    // entity destructor reads its parent or children.
    for (auto* entity : batch)
    {
        auto it = std::ranges::find(rootEntities, entity);
        if (it != rootEntities.end()) rootEntities.erase(it);
        std::erase(pendingAwake, entity);   // destroyed before it ever awoke
        allAliveEntities.erase(entity);
    }

    markedForDestroy.clear();

    // The walk order holds raw pointers - never let it outlive a free.
    entityOrderDirty = true;
}

void MScene::addToRoot(MSpatialEntity* entity) {
    if (entity->getParent() != nullptr)
        return;
    auto it = std::find(rootEntities.begin(), rootEntities.end(), entity);
    if (it != rootEntities.end())
        return;
    rootEntities.push_back(entity);
    entityOrderDirty = true;
}

void MScene::insertRootEntityAt(MSpatialEntity* entity, int index)
{
    if (!entity)
        return;

    // Remove from current parent if any.
    if (entity->getParent())
    {
        auto& siblings = entity->getParent()->getChildren();
        auto it = std::find(siblings.begin(), siblings.end(), entity);
        if (it != siblings.end())
            siblings.erase(it);
        // Clear parent pointer - can't call setParent(nullptr) as that
        // would call addToSceneRoot which appends to the end.
    }

    // Remove from root list if already present.
    auto it = std::find(rootEntities.begin(), rootEntities.end(), entity);
    if (it != rootEntities.end())
        rootEntities.erase(it);

    // Clamp and insert.
    index = std::clamp(index, 0, (int)rootEntities.size());
    rootEntities.insert(rootEntities.begin() + index, entity);
    entityOrderDirty = true;
}

MSpatialEntity* MScene::find(const SString& path)
{
    std::string p = path.str();
    if (p.starts_with("./"))
        p = p.substr(2);

    if (p.empty()) return nullptr;

    // First segment matches against root entities
    size_t slash = p.find('/');
    std::string first = (slash != std::string::npos) ? p.substr(0, slash) : p;

    for (auto* root : rootEntities)
    {
        if (root && root->getName().str() == first)
        {
            if (slash == std::string::npos)
                return root;

            // Delegate remaining path to the entity's find
            return root->find(SString(p.substr(slash + 1)));
        }
    }
    return nullptr;
}

bool MScene::tryParse(pugi::xml_document* doc) {
    if (doc == nullptr)
        return false;

    if (!doc->child(VALID_SCENE_FILE_XML_TAG.c_str()))
        return false;

    auto scene_xml = doc->child(VALID_SCENE_FILE_XML_TAG.c_str());
    if(scene_xml.attribute(SCENE_NAME_TAG.c_str()) != nullptr) {
        name = scene_xml.attribute(SCENE_NAME_TAG.c_str()).value();
    }
    else
        name = "Untitled";

    for (auto& child : scene_xml.children()) {
        recursivelyLoadEntity(child, nullptr);
    }

    for(auto& entity : rootEntities) {
        entity->updateTransforms();
    }

    startScene();
    return true;
}

void MScene::recursivelyLoadEntity(pugi::xml_node currNode, MSpatialEntity* parent)
{
    if (currNode == nullptr)
    {
        return;
    }
    if (MSceneEntityTypeDeserializer::ATTRIB_NODE == SString(currNode.name()))
    {
        return;
    }

    auto type = currNode.name();
    auto deserializer = MSceneEntityTypeMap::getDeserializer(type);
    if (deserializer == nullptr)
    {
        MWARN("cannot find deserializer for {}, skipping this branch." + std::string(type));
        return;
    }

    MSpatialEntity* entity = deserializer->deserialize(currNode);
    if (entity == nullptr)
    {
        MWARN("deserialized entity for {} is nullptr, skipping this branch." + std::string(type));
        return;
    }
    if (parent != nullptr)
        parent->addChild(entity);

    for (auto child : currNode.children())
    {
        recursivelyLoadEntity(child, entity);
    }
}

bool MScene::registerEntity(MSpatialEntity* entity)
{
    if (entity == nullptr) return false;

    addToRoot(entity);
    allAliveEntities[entity] = entity;
    entityOrderDirty = true;
    pendingAwake.push_back(entity);
    entity->onCreate();
    return true;
}

static void appendSubtree(MSpatialEntity* entity, std::vector<MSpatialEntity*>& out)
{
    if (entity == nullptr)
        return;
    out.push_back(entity);
    for (auto* child : entity->getChildren())
        appendSubtree(child, out);
}

const std::vector<MSpatialEntity*>& MScene::getEntityOrder()
{
    // Rebuilt only here, at the start of a pass - never mid-pass. Entities
    // created during a pass join from the next one; destroyed ones are only
    // marked until destroyMarked() frees them (and dirties this list).
    if (entityOrderDirty)
    {
        entityOrder.clear();
        entityOrder.reserve(allAliveEntities.size());
        for (auto* root : rootEntities)
            appendSubtree(root, entityOrder);
        entityOrderDirty = false;
    }
    return entityOrder;
}