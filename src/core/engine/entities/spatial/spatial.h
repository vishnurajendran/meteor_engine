//
// spatial.h
//

#pragma once
#ifndef SPATIAL_H
#define SPATIAL_H

#include <vector>
#include "core/engine/assetmanagement/asset/field_asset_ref_types.h"
#include "core/engine/entities/entity_type_registry.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "core/engine/scripting/interface/script_instance.h"
#include "core/engine/scripting/lua/lua_script_asset.h"
#include "core/engine/scripting/script_binding_macros.h"
#include "core/utils/field_engine_types.h"
#include "data/serialized_class_base.h"
#include "entityflags.h"
#include "spatial_class_macros.h"

SCRIPT_BIND_CLASS()
class MSpatialEntity : public MObject, public SerializedClassBase
{
private:
    DEFINE_SPATIAL_CLASS(MSpatialEntity)
    DECLARE_FIELD(scriptReference, TAssetRef<MLuaScriptAsset>, {})

public:
    static MSpatialEntity* createInstance(const SString& name = {});

    template <typename T>
    static T* createInstance(const SString& name, MScene* ownerScene = nullptr)
    {
        static_assert(std::is_base_of<MSpatialEntity, T>::value, "T must derive from MSpatialEntity");
        T* entity = new T();
        entity->setName(name);
        if (!ownerScene)
        {
            ownerScene = MSceneManager::getSceneManagerInstance()->getActiveScene();
            if (!ownerScene)
                MERROR("NO SCENE OPEN");
        }
        ownerScene->registerEntity(entity);
        entity->ownerScene = ownerScene;
        return entity;
    }

    // Scene serialization
    pugi::xml_node serialiseEntity(pugi::xml_node parent) const;
    static MSpatialEntity* deserialiseEntity(const pugi::xml_node& node);

    SCRIPT_BIND_FUNC()
    void destroy();

    // Flags / enable
    [[nodiscard]] EEntityFlags getEntityFlags() const { return flags; }
    void setEntityFlags(EEntityFlags flag) { flags = flag; }

    SCRIPT_BIND_FUNC()
    void setEnabled(bool enable);

    SCRIPT_BIND_FUNC()
    [[nodiscard]] bool getEnabled() const { return enabled; }

    [[nodiscard]] bool isEnabledInHierarchy() const;

    // Transform getters
    SCRIPT_BIND_FUNC()
    [[nodiscard]] SVector3 getRelativePosition() const { return relativePosition; }

    SCRIPT_BIND_FUNC()
    [[nodiscard]] SVector3 getRelativeScale() const { return relativeScale; }

    SCRIPT_BIND_FUNC()
    [[nodiscard]] SQuaternion getRelativeRotation() const { return relativeRotation; }

    SCRIPT_BIND_FUNC()
    [[nodiscard]] SVector3 getWorldPosition() const;

    SCRIPT_BIND_FUNC()
    [[nodiscard]] SQuaternion getWorldRotation() const;

    SCRIPT_BIND_FUNC()
    [[nodiscard]] SVector3 getWorldScale() const;

    [[nodiscard]] SMatrix4 getModelMatrix() const { return modelMatrix; }
    [[nodiscard]] SMatrix4 getTransformMatrix() const { return modelMatrix; }

    // Transform setters
    SCRIPT_BIND_FUNC()
    void setRelativePosition(const SVector3& p)
    {
        relativePosition = p;
        updateTransforms();
    }

    SCRIPT_BIND_FUNC()
    void setRelativeRotation(const SQuaternion& r)
    {
        relativeRotation = r;
        updateTransforms();
    }

    SCRIPT_BIND_FUNC()
    void setRelativeScale(const SVector3& s)
    {
        relativeScale = s;
        updateTransforms();
    }

    SCRIPT_BIND_FUNC()
    void setWorldPosition(const SVector3& worldPosition);

    SCRIPT_BIND_FUNC()
    void setWorldRotation(const SQuaternion& worldRotation);

    SCRIPT_BIND_FUNC()
    [[nodiscard]] SVector3 getForwardVector() const;

    SCRIPT_BIND_FUNC()
    [[nodiscard]] SVector3 getRightVector() const;

    SCRIPT_BIND_FUNC()
    [[nodiscard]] SVector3 getUpVector() const;

    [[nodiscard]] MSpatialEntity* getParent() { return parent; }
    [[nodiscard]] std::vector<MSpatialEntity*>& getChildren() { return children; }

    void setParentScene(MScene* scene);

    void setParent(MSpatialEntity* newParent);
    void addChild(MSpatialEntity* entity);
    void removeChild(MSpatialEntity* entity);

    /// Insert entity as a child at a specific index (for hierarchy reordering).
    /// Removes from its current parent first. Clamps index to [0, children.size()].
    void insertChildAt(MSpatialEntity* entity, int index);

    SCRIPT_BIND_FUNC()
    MSpatialEntity* find(const SString& path);


    template <typename T>
    T* findT(const SString& searchName)
    {
        static_assert(std::is_base_of<MSpatialEntity, T>::value, "T must derive from MSpatialEntity");
        if (name == searchName)
            if (T* typed = dynamic_cast<T*>(this))
                return typed;
        for (auto* child : children)
            if (child)
                if (T* result = child->findT<T>(searchName))
                    return result;
        return nullptr;
    }

    virtual void updateTransforms();

    virtual void onCreate();
    virtual void onStart();
    virtual void onUpdate(float deltaTime);
    virtual void onFixedUpdate(float fixedDeltaTime);
    virtual void onExit();
    virtual void onDrawGizmo(SVector2 renderResolution);
    virtual void onEnable() {} // no impl here
    virtual void onDisable() {} // no impl here

    [[nodiscard]] bool hasStarted() const { return entityStarted; }
    [[nodiscard]] bool getCanTick() const { return canTick; }
    void setCanTick(bool tick) { canTick = tick; }

    // Script reference -- used by the inspector to get/set the assigned script.
    // The setter releases any running script instance so the new asset takes
    // effect on the next tick.
    [[nodiscard]] MAsset* getScriptAssetRef() const;
    [[nodiscard]] IScriptInstance* getScriptInstance() const { return scriptInstance; }
    void setScriptAssetRef(MAsset* asset);

    // Returns the stored asset ID from the script reference field directly,
    // without resolving through the asset manager. Safe to call even when the
    // asset manager subsystem is unavailable.
    [[nodiscard]] SString getScriptAssetId() const;
    ~MSpatialEntity() override = default;

private:
    void ensureScript();
    void scriptStart();
    void scriptTick(float dt);
    void scriptFixedTick(float fdt);
    void scriptStop();

protected:
    void onSerialise(pugi::xml_node& node) override;
    void onDeserialise(const pugi::xml_node& node) override;

    MSpatialEntity();
    explicit MSpatialEntity(MSpatialEntity* parent);

    SVector3 relativePosition = SVector3(0.0f);
    SQuaternion relativeRotation = glm::identity<SQuaternion>();
    SVector3 relativeScale = SVector3(1.0f);
    SMatrix4 modelMatrix = glm::identity<SMatrix4>();

    MSpatialEntity* parent = nullptr;
    std::vector<MSpatialEntity*> children;
    bool enabled = true;
    EEntityFlags flags = EEntityFlags::Default;
    MScene* ownerScene = nullptr;

private:
    [[nodiscard]] SMatrix4 computeLocalMatrix() const;
    static SString generateName(const SString& base);
    void propagateActiveState(bool active);

    bool canTick = false;
    bool entityStarted = false;
    IScriptInstance* scriptInstance = nullptr;
    bool onExitCalled = false;
};

#endif // SPATIAL_H