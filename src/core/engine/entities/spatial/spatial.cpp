#include "spatial.h"

#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"

// ─── Static member definitions ───────────────────────────────────────────────

std::unordered_map<MObject*, MObjectPtr<MSpatialEntity>> MSpatialEntity::allAliveEntities;
std::vector<MSpatialEntity*>                              MSpatialEntity::markedForDestruction;

// ─── Internal scene-root helpers ─────────────────────────────────────────────

static MScene* activeScene()
{
    return MSceneManager::getSceneManagerInstance()->getActiveScene();
}

static void addToSceneRoot(MSpatialEntity* entity)
{
    if (entity->getParent() != nullptr)
        entity->getParent()->removeChild(entity);

    if (auto* scene = activeScene())
        scene->addToRoot(entity);
}

// NOTE: getRootEntities() must return a non-const reference for this to compile.
// If it returns by value, add a removeFromRoot(MSpatialEntity*) method to MScene.
static void removeFromSceneRoot(const MSpatialEntity* entity)
{
    auto* scene = activeScene();
    if (!scene) return;

    auto& roots = scene->getRootEntities();          // reference, not a copy
    auto  it    = std::find(roots.begin(), roots.end(), entity);
    if (it != roots.end())
        roots.erase(it);
}

// ─── Factory ─────────────────────────────────────────────────────────────────

MSpatialEntity* MSpatialEntity::createInstance(const SString& name)
{
    // Protected constructor → only reachable here and from subclass factories.
    auto* entity = new MSpatialEntity(nullptr);
    entity->setName(name.empty() ? generateName(STR("SpatialEntity")) : name);
    addAliveEntity(entity);
    return entity;
}

SString MSpatialEntity::generateName(const SString& base)
{
    // Check if plain base name is already taken; if so, append (1), (2), …
    auto taken = [&](const SString& candidate) -> bool
    {
        for (const auto& [ptr, mop] : allAliveEntities)
            if (mop->getName() == candidate) return true;
        return false;
    };

    if (!taken(base)) return base;

    for (int i = 1; ; ++i)
    {
        SString candidate = base + STR(" (") + SString::fromInt(i) + STR(")");
        if (!taken(candidate)) return candidate;
    }
}

// ─── Constructors / Destructor ────────────────────────────────────────────────

MSpatialEntity::MSpatialEntity() : MSpatialEntity(nullptr) {}

MSpatialEntity::MSpatialEntity(MSpatialEntity* parentEntity)
{
    name = "SpatialEntity";

    if (parentEntity)
        parentEntity->addChild(this);      // addChild calls setParent internally
    else
        addToSceneRoot(this);

    onStart();
}

MSpatialEntity::~MSpatialEntity()
{
    // onExit is called by destroy() before queuing for deletion, so game-code
    // cleanup always happens before the destructor runs.
}

// ─── Hierarchy ────────────────────────────────────────────────────────────────

void MSpatialEntity::setParent(MSpatialEntity* newParent)
{
    if (newParent == parent) return;

    if (newParent)
    {
        // addChild handles detaching from the old parent and root list.
        newParent->addChild(this);
    }
    else
    {
        // Detach from current parent → become a scene root.
        if (parent)
            parent->removeChild(this);   // removeChild calls addToSceneRoot
    }
}

void MSpatialEntity::addChild(MSpatialEntity* entity)
{
    if (!entity || entity == this) return;

    // Already a child — nothing to do.
    if (std::ranges::find(children, entity) != children.end()) return;

    if (entity->parent)
    {
        // Silent detach from old parent: erase from sibling list and clear the
        // parent pointer WITHOUT calling removeChild, which would promote the
        // entity to a scene root and cause it to appear in both the root list
        // and the new parent's children at the same time.
        auto& siblings = entity->parent->children;
        auto  it       = std::ranges::find(siblings, entity);
        if (it != siblings.end()) siblings.erase(it);
        entity->parent = nullptr;
    }
    else
    {
        // Entity was a scene root — remove it from that list before parenting.
        removeFromSceneRoot(entity);
    }

    children.push_back(entity);
    entity->parent = this;
    entity->updateTransforms();
}

void MSpatialEntity::removeChild(MSpatialEntity* entity)
{
    if (!entity) return;

    auto it = std::ranges::find(children, entity);
    if (it == children.end()) return;

    children.erase(it);
    entity->parent = nullptr;
    addToSceneRoot(entity);             // promote to scene root
    entity->updateTransforms();
}

// ─── Transform ───────────────────────────────────────────────────────────────

SMatrix4 MSpatialEntity::computeLocalMatrix() const
{
    SMatrix4 T = glm::translate(SMatrix4(1.0f), relativePosition);
    SMatrix4 R = glm::mat4_cast(relativeRotation);
    SMatrix4 S = glm::scale(SMatrix4(1.0f), relativeScale);
    return T * R * S;
}

void MSpatialEntity::updateTransforms()
{
    // modelMatrix stores the WORLD-space transform so that getWorldPosition /
    // getWorldRotation can read it cheaply without walking up the hierarchy.
    SMatrix4 local = computeLocalMatrix();
    modelMatrix = parent ? parent->modelMatrix * local : local;

    for (auto* child : children)
        if (child) child->updateTransforms();
}

SVector3 MSpatialEntity::getWorldPosition() const
{
    return SVector3(modelMatrix[3]);
}

SQuaternion MSpatialEntity::getWorldRotation() const
{
    // Strip scale from the upper-left 3×3 before converting to quaternion.
    glm::mat3 rot = glm::mat3(modelMatrix);
    rot[0] = glm::normalize(rot[0]);
    rot[1] = glm::normalize(rot[1]);
    rot[2] = glm::normalize(rot[2]);
    return glm::quat_cast(rot);
}

void MSpatialEntity::setWorldPosition(const SVector3& worldPosition)
{
    if (parent)
    {
        SMatrix4 invParent    = glm::inverse(parent->modelMatrix);
        SVector4 localPos4    = invParent * SVector4(worldPosition, 1.0f);
        relativePosition      = SVector3(localPos4);
    }
    else
    {
        relativePosition = worldPosition;
    }
    updateTransforms();
}

void MSpatialEntity::setWorldRotation(const SQuaternion& worldRotation)
{
    if (parent)
        relativeRotation = glm::inverse(parent->getWorldRotation()) * worldRotation;
    else
        relativeRotation = worldRotation;

    updateTransforms();
}

SVector3 MSpatialEntity::getForwardVector() const
{
    return glm::normalize(getWorldRotation() * SVector3(0.0f, 0.0f, -1.0f));
}

SVector3 MSpatialEntity::getRightVector() const
{
    return glm::normalize(getWorldRotation() * SVector3(1.0f, 0.0f, 0.0f));
}

SVector3 MSpatialEntity::getUpVector() const
{
    return glm::normalize(getWorldRotation() * SVector3(0.0f, 1.0f, 0.0f));
}

// ─── Update loop ─────────────────────────────────────────────────────────────

void MSpatialEntity::updateAllSceneEntities(float deltaTime)
{
    // Snapshot the keys so that entities created/destroyed mid-update
    // don't invalidate the iterator.
    std::vector<MObject*> keys;
    keys.reserve(allAliveEntities.size());
    for (const auto& [ptr, _] : allAliveEntities)
        keys.push_back(ptr);

    for (auto* key : keys)
    {
        auto it = allAliveEntities.find(key);
        if (it != allAliveEntities.end())
            it->second->onUpdate(deltaTime);
    }
}

// ─── Destroy ─────────────────────────────────────────────────────────────────

void MSpatialEntity::destroy(MSpatialEntity* entity)
{
    if (!entity) return;

    entity->onExit();

    // Re-parent or promote children before the entity disappears.
    if (entity->parent)
    {
        for (auto* child : entity->children)
            entity->parent->addChild(child);    // reparent up

        entity->parent->removeChild(entity);
    }
    else
    {
        for (auto* child : entity->children)
            addToSceneRoot(child);              // promote each child to root

        // Remove this entity from the scene root list.
        // Use reference — NOT a copy.
        auto* scene = activeScene();
        if (scene)
        {
            auto& roots = scene->getRootEntities();
            auto  it    = std::ranges::find(roots, entity);
            if (it != roots.end()) roots.erase(it);
        }
    }

    entity->children.clear();
    entity->parent = nullptr;

    markedForDestruction.push_back(entity);
}

void MSpatialEntity::destroyMarked()
{
    for (auto* marked : markedForDestruction)
    {
        // Erasing from the map drops the MObjectPtr, which decrements the GC
        // ref-count. When it reaches zero the GC calls delete automatically.
        removeFromAliveEntities(marked);
    }
    markedForDestruction.clear();
}

// ─── Alive-entity registry ────────────────────────────────────────────────────

void MSpatialEntity::addAliveEntity(MSpatialEntity* entity)
{
    // Inserting an MObjectPtr calls MGarbageCollector::reference -> ref-count = 1.
    allAliveEntities.emplace(entity, MObjectPtr<MSpatialEntity>(entity));
}

void MSpatialEntity::removeFromAliveEntities(MSpatialEntity* entity)
{
    // Erasing destroys the MObjectPtr -> MGarbageCollector::dereference.
    // If ref-count hits zero the GC calls delete.
    allAliveEntities.erase(entity);
}

void MSpatialEntity::onStart()  {}
void MSpatialEntity::onUpdate(float) {}
void MSpatialEntity::onExit()   {}
void MSpatialEntity::onDrawGizmo(SVector2) {}