#include "spatial.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"


IMPLEMENT_CLASS(MSpatialEntity)

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

static void removeFromSceneRoot(const MSpatialEntity* entity)
{
    auto* scene = activeScene();
    if (!scene) return;
    auto& roots = scene->getRootEntities();
    auto  it    = std::find(roots.begin(), roots.end(), entity);
    if (it != roots.end()) roots.erase(it);
}


MSpatialEntity* MSpatialEntity::createInstance(const SString& name)
{
    auto* entity = new MSpatialEntity(nullptr);
    entity->setName(name.empty() ? generateName(STR("MSpatialEntity")) : name);
    auto* scene  = MSceneManager::getSceneManagerInstance()->getActiveScene();
    entity->ownerScene = scene;
    scene->registerEntity(entity);
    return entity;
}

SString MSpatialEntity::generateName(const SString& base)
{
    auto* scene = MSceneManager::getSceneManagerInstance()->getActiveScene();
    auto taken  = [&](const SString& candidate) -> bool {
        for (const auto& [ptr, mop] : scene->getAllEntities())
            if (mop->getName() == candidate) return true;
        return false;
    };
    if (!taken(base)) return base;
    for (int i = 1; ; ++i) {
        SString candidate = base + STR(" (") + SString::fromInt(i) + STR(")");
        if (!taken(candidate)) return candidate;
    }
}

MSpatialEntity::MSpatialEntity() : MSpatialEntity(nullptr) {}

MSpatialEntity::MSpatialEntity(MSpatialEntity* parentEntity)
{
    name = "MSpatialEntity";
    if (parentEntity) parentEntity->addChild(this);
    else              addToSceneRoot(this);
}

void MSpatialEntity::setParent(MSpatialEntity* newParent)
{
    if (newParent == parent) return;
    if (newParent) newParent->addChild(this);
    else if (parent) parent->removeChild(this);
}

void MSpatialEntity::addChild(MSpatialEntity* entity)
{
    if (!entity || entity == this) return;
    if (std::ranges::find(children, entity) != children.end()) return;

    if (entity->parent) {
        auto& siblings = entity->parent->children;
        auto  it       = std::ranges::find(siblings, entity);
        if (it != siblings.end()) siblings.erase(it);
        entity->parent = nullptr;
    } else {
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
    addToSceneRoot(entity);
    entity->updateTransforms();
}


SMatrix4 MSpatialEntity::computeLocalMatrix() const
{
    return glm::translate(SMatrix4(1.0f), relativePosition)
         * glm::mat4_cast(relativeRotation)
         * glm::scale(SMatrix4(1.0f), relativeScale);
}

void MSpatialEntity::updateTransforms()
{
    SMatrix4 local = computeLocalMatrix();
    modelMatrix = parent ? parent->modelMatrix * local : local;
    for (auto* child : children)
        if (child) child->updateTransforms();
}

SVector3 MSpatialEntity::getWorldPosition() const { return SVector3(modelMatrix[3]); }

SQuaternion MSpatialEntity::getWorldRotation() const
{
    glm::mat3 rot = glm::mat3(modelMatrix);
    rot[0] = glm::normalize(rot[0]);
    rot[1] = glm::normalize(rot[1]);
    rot[2] = glm::normalize(rot[2]);
    return glm::quat_cast(rot);
}

void MSpatialEntity::setWorldPosition(const SVector3& worldPosition)
{
    relativePosition = parent
        ? SVector3(glm::inverse(parent->modelMatrix) * SVector4(worldPosition, 1.0f))
        : worldPosition;
    updateTransforms();
}

void MSpatialEntity::setWorldRotation(const SQuaternion& worldRotation)
{
    relativeRotation = parent
        ? glm::inverse(parent->getWorldRotation()) * worldRotation
        : worldRotation;
    updateTransforms();
}

SVector3 MSpatialEntity::getForwardVector() const
    { return glm::normalize(getWorldRotation() * SVector3(0, 0, -1)); }
SVector3 MSpatialEntity::getRightVector() const
    { return glm::normalize(getWorldRotation() * SVector3(1, 0, 0)); }
SVector3 MSpatialEntity::getUpVector() const
    { return glm::normalize(getWorldRotation() * SVector3(0, 1, 0)); }


void MSpatialEntity::onSerialise(pugi::xml_node& node)
{
    node.append_attribute("name")    = getName().c_str();
    node.append_attribute("enabled") = enabled;
    node.append_attribute("flags")   = static_cast<uint32_t>(flags);

    std::vector<FieldBase*> tmp;
    Field<SVector3>    pos  (tmp, "relativePosition", relativePosition);
    Field<SQuaternion> rot  (tmp, "relativeRotation", relativeRotation);
    Field<SVector3>    scale(tmp, "relativeScale",    relativeScale);
    for (auto* f : tmp) f->write(node);
}

void MSpatialEntity::onDeserialise(const pugi::xml_node& node)
{
    if (auto a = node.attribute("name"))    setName(SString(a.value()));
    if (auto a = node.attribute("enabled")) enabled = a.as_bool(true);
    if (auto a = node.attribute("flags"))   flags   = static_cast<EEntityFlags>(a.as_uint());

    std::vector<FieldBase*> tmp;
    Field<SVector3>    pos  (tmp, "relativePosition", relativePosition);
    Field<SQuaternion> rot  (tmp, "relativeRotation", relativeRotation);
    Field<SVector3>    scale(tmp, "relativeScale",    relativeScale);
    for (auto* f : tmp) f->load(node);

    relativePosition = pos.get();
    relativeRotation = rot.get();
    relativeScale    = scale.get();
    updateTransforms();
}

pugi::xml_node MSpatialEntity::serialiseEntity(pugi::xml_node parent) const
{
    pugi::xml_node node = parent.append_child("entity");
    node.append_attribute("type") = typeInfo().name;
    const_cast<MSpatialEntity*>(this)->serialiseToNode(node);

    if (!children.empty())
    {
        pugi::xml_node childrenNode = node.append_child("children");
        for (const auto* child : children)
            if (child) child->serialiseEntity(childrenNode);
    }
    return node;
}

MSpatialEntity* MSpatialEntity::deserialiseEntity(const pugi::xml_node& node)
{
    const std::string type = node.attribute("type").as_string("MSpatialEntity");
    MSpatialEntity* entity = MEntityTypeRegistry::get().create(type);

    if (!entity) {
        MERROR(SString::format("[Scene] Unknown type '{0}', falling back to MSpatialEntity", type));
        entity = MSpatialEntity::createInstance();
    }

    entity->deserialiseFromNode(node);

    if (pugi::xml_node childrenNode = node.child("children"))
        for (pugi::xml_node childNode : childrenNode.children("entity"))
            if (auto* child = deserialiseEntity(childNode))
                entity->addChild(child);

    return entity;
}

void MSpatialEntity::destroy()
{
    if (!ownerScene) {
        MERROR(SString::format("MSpatialEntity:: Owner Scene is null for {0}.", getName()));
        return;
    }
    onExit();
    ownerScene->markForDestroy(this);
}

void MSpatialEntity::onCreate()  {}
void MSpatialEntity::onStart()   { entityStarted = true; }
void MSpatialEntity::onUpdate(float) {}
void MSpatialEntity::onExit()    {}
void MSpatialEntity::onDrawGizmo(SVector2) {}