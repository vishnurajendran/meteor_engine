#include "spatial.h"

#include "core/application/application.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "core/engine/scripting/interface/scripting_engine_interface.h"
#include "core/engine/scripting/scripting_call_symbols.h"
#include "core/engine/composition/composition_utility.h"


IMPLEMENT_SPATIAL_CLASS(MSpatialEntity)

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

    canTick = true;
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

SVector3 MSpatialEntity::getWorldScale() const
{
    // Each column's length encodes the scale applied along that axis.
    return SVector3(
        glm::length(SVector3(modelMatrix[0])),
        glm::length(SVector3(modelMatrix[1])),
        glm::length(SVector3(modelMatrix[2]))
    );
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
SVector3 MSpatialEntity::getUpVector() const { return glm::normalize(getWorldRotation() * SVector3(0, 1, 0)); }


void MSpatialEntity::ensureScript()
{
    if (scriptInstance != nullptr)
        return;

    if (scriptReference.get().isEmpty())
        return;

    auto* engine = MEngineSubsystemRegistry::getSubsystem<IScriptingEngineSubsystem>();
    if (engine == nullptr)
    {
        MERROR("MSpatialEntity::ensureScript: scripting engine is null");
        return;
    }

    scriptInstance = engine->createScriptInstance(scriptReference.get().resolve(), this);

    // A valid script needs per-frame updates so the scene loop
    // calls onUpdate, which drives scriptTick / onTick().
    if (scriptInstance != nullptr)
        canTick = true;
}

void MSpatialEntity::scriptStart()
{
    if (!MApplication::getAppInstance()->isSimulating())
        return;
    ensureScript();
    if (scriptInstance)
    {
        SDynValue outVar;
        scriptInstance->callFunc(ScriptinCallSymbols::SCRIPT_FUNC_START, std::vector<SDynValue>{}, outVar);
    }
}

void MSpatialEntity::scriptTick(float dt)
{
    if (!MApplication::getAppInstance()->isSimulating())
        return;

    if (!scriptInstance)
    {
        ensureScript();
        if (scriptInstance)
            scriptStart();
    }

    if (scriptInstance)
    {
        SDynValue outVar;
        SDynValue dtVar;
        dtVar.setFloat(dt);
        scriptInstance->callFunc(ScriptinCallSymbols::SCRIPT_FUNC_TICK, std::vector<SDynValue>{dtVar}, outVar);
    }
}

void MSpatialEntity::scriptFixedTick(float fdt)
{
    if (!MApplication::getAppInstance()->isSimulating())
        return;

    if (!scriptInstance)
    {
        ensureScript();
        if (scriptInstance)
            scriptStart();
    }

    if (scriptInstance)
    {
        SDynValue outVar;
        SDynValue dtVar;
        dtVar.setFloat(fdt);
        scriptInstance->callFunc(ScriptinCallSymbols::SCRIPT_FUNC_FIXED_TICK, std::vector<SDynValue>{dtVar}, outVar);
    }
}

void MSpatialEntity::scriptStop()
{
    if (!MApplication::getAppInstance()->isSimulating())
        return;

    if (scriptInstance)
    {
        SDynValue outVar;
        scriptInstance->callFunc(ScriptinCallSymbols::SCRIPT_FUNC_STOP, std::vector<SDynValue>{}, outVar);
    }
}


// ---------------------------------------------------------------------------
// Script reference accessors (used by the inspector)
// ---------------------------------------------------------------------------

MAsset* MSpatialEntity::getScriptAssetRef() const
{
    // resolve() does GUID-first lookup with path fallback,
    // which handles legacy scene files and copied directories
    return scriptReference.get().resolve();
}

void MSpatialEntity::setScriptAssetRef(MAsset* asset)
{
    // Release the previous script instance so ensureScript() will
    // create a fresh one from the new asset on the next tick
    if (scriptInstance)
    {
        scriptStop();
        auto* engine = MEngineSubsystemRegistry::getSubsystem<IScriptingEngineSubsystem>();
        if (engine)
            engine->releaseScriptInstance(scriptInstance);
        scriptInstance = nullptr;
    }

    // TAssetRef's raw-pointer constructor captures both GUID and path.
    // Passing nullptr clears both fields, which represents "no script".
    scriptReference.set(TAssetRef<MLuaScriptAsset>(dynamic_cast<MLuaScriptAsset*>(asset)));
}

SString MSpatialEntity::getScriptAssetId() const
{
    return scriptReference.get().getAssetId();
}

void MSpatialEntity::onSerialise(pugi::xml_node& node)
{
    node.append_attribute("name")    = getName().c_str();
    node.append_attribute("enabled") = enabled;
    node.append_attribute("flags")   = static_cast<uint32_t>(flags);

    // Composition change-tracking — only on instance roots.
    if (!compAssetReference.get().isEmpty())
    {
        node.append_attribute(MCompositionAsset::SOURCE_HASH_ATTRIBUTE) = compSourceHash.c_str();
        node.append_attribute(MCompositionAsset::STATE_HASH_ATTRIBUTE)  = compStateHash.c_str();
    }

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

    compSourceHash = SString(node.attribute(MCompositionAsset::SOURCE_HASH_ATTRIBUTE).as_string(""));
    compStateHash  = SString(node.attribute(MCompositionAsset::STATE_HASH_ATTRIBUTE).as_string(""));

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

MSpatialEntity* MSpatialEntity::deserialiseEntity(const pugi::xml_node& node, bool resolveCompositions)
{
    // A clean composition instance whose asset changed is built from the
    // .comp (keeping its root name / transform) — its saved children are stale.
    if (resolveCompositions)
        if (MSpatialEntity* fromAsset = MCompositionUtility::tryBuildFromAsset(node))
            return fromAsset;

    const std::string type = node.attribute("type").as_string("MSpatialEntity");
    MSpatialEntity* entity = MEntityTypeRegistry::get().create(type);

    if (!entity) {
        MERROR(SString::format("[Scene] Unknown type '{0}', falling back to MSpatialEntity", type));
        entity = MSpatialEntity::createInstance();
    }

    entity->deserialiseFromNode(node);

    if (pugi::xml_node childrenNode = node.child("children"))
        for (pugi::xml_node childNode : childrenNode.children("entity"))
            if (auto* child = deserialiseEntity(childNode, resolveCompositions))
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

void MSpatialEntity::insertChildAt(MSpatialEntity* entity, int index)
{
    if (!entity || entity == this)
        return;

    // Remove from current parent.
    if (entity->parent)
    {
        auto& siblings = entity->parent->children;
        auto it = std::ranges::find(siblings, entity);
        if (it != siblings.end())
            siblings.erase(it);
        entity->parent = nullptr;
    }
    else
    {
        // Remove from scene root list.
        removeFromSceneRoot(entity);
    }

    // Clamp and insert at position.
    index = std::clamp(index, 0, (int)children.size());
    children.insert(children.begin() + index, entity);
    entity->parent = this;
    entity->updateTransforms();
}

MSpatialEntity* MSpatialEntity::find(const SString& path)
{
    std::string p = path.str();

    // Strip "./" prefix if present
    if (p.starts_with("./"))
        p = p.substr(2);

    if (p.empty()) return nullptr;

    // Traverse children step by step along each "/" segment
    MSpatialEntity* current = this;
    size_t start = 0;

    while (start < p.size())
    {
        size_t slash = p.find('/', start);
        std::string segment = (slash != std::string::npos)
            ? p.substr(start, slash - start)
            : p.substr(start);
        start = (slash != std::string::npos) ? slash + 1 : p.size();

        if (segment.empty()) continue;

        bool found = false;
        for (auto* child : current->children)
        {
            if (child && child->getName().str() == segment)
            {
                current = child;
                found = true;
                break;
            }
        }
        if (!found) return nullptr;
    }

    return current == this ? nullptr : current;
}

void MSpatialEntity::onCreate()  {}
void MSpatialEntity::onStart()
{
    entityStarted = true;
    scriptStart();
}

void MSpatialEntity::onUpdate(const float deltaTime)
{
    scriptTick(deltaTime);
}

void MSpatialEntity::onFixedUpdate(const float fixedDeltaTime)
{
    scriptFixedTick(fixedDeltaTime);
}


void MSpatialEntity::onExit()
{
    onExitCalled = true;
    scriptStop();
}

void MSpatialEntity::onDrawGizmo(SVector2)
{

}

void MSpatialEntity::setEnabled(bool enable)
{
    if (enabled == enable) return;

    bool wasActive = isEnabledInHierarchy();
    enabled = enable;
    bool nowActive = isEnabledInHierarchy();

    if (wasActive != nowActive)
        propagateActiveState(nowActive);
}

bool MSpatialEntity::isEnabledInHierarchy() const
{
    if (!enabled) return false;
    if (parent)   return parent->isEnabledInHierarchy();
    return true;
}

void MSpatialEntity::propagateActiveState(bool active)
{
    if (active)
        onEnable();
    else
        onDisable();

    for (auto* child : children)
    {
        // Only propagate to children whose own flag is true -
        // individually disabled children stay disabled regardless.
        if (child && child->enabled)
            child->propagateActiveState(active);
    }
}