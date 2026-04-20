//
// Created by ssj5v on 26-09-2024.
//
#pragma once
#ifndef SPATIAL_H
#define SPATIAL_H
#include <core/meteor_core_minimal.h>
#include <vector>

#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "entityflags.h"

class MSpatialEntity : public MObject
{
    // ── Factory ───────────────────────────────────────────────────────────────
public:
    // Primary way to create an entity. Name auto-generates as
    // "SpatialEntity", "SpatialEntity (1)", "SpatialEntity (2)" … when empty.
    static MSpatialEntity* createInstance(const SString& name = {});

    // Typed variant for subclasses: e.g. createInstance<MCamera>("MainCam")
    template<typename T>
    static T* createInstance(const SString& name, MScene* ownerScene=nullptr)
    {
        static_assert(std::is_base_of<MSpatialEntity, T>::value,
                      "T must derive from MSpatialEntity");
        T* entity = new T();
        entity->setName(name);
        if (ownerScene == nullptr)
            ownerScene = MSceneManager::getSceneManagerInstance()->getActiveScene();

        ownerScene->registerEntity(entity);
        entity->ownerScene = ownerScene;
        return entity;
    }

    // ── Lifetime ──────────────────────────────────────────────────────────────
    void destroy();

    // ── Flags / enable ────────────────────────────────────────────────────────
    [[nodiscard]] EEntityFlags getEntityFlags() const { return flags; }
    void setEntityFlags(EEntityFlags flag)             { flags = flag; }

    void setEnabled(bool enable)               { enabled = enable; }
    [[nodiscard]] bool getEnabled() const      { return enabled; }

    // ── Transform getters ─────────────────────────────────────────────────────
    [[nodiscard]] SVector3    getRelativePosition() const { return relativePosition; }
    [[nodiscard]] SVector3    getRelativeScale()    const { return relativeScale; }
    [[nodiscard]] SQuaternion getRelativeRotation() const { return relativeRotation; }

    // World-space values read from the cached world matrix (kept up-to-date by updateTransforms)
    [[nodiscard]] SVector3    getWorldPosition() const;
    [[nodiscard]] SQuaternion getWorldRotation() const;

    // Full world-space 4×4 matrix (= the cached modelMatrix).
    [[nodiscard]] SMatrix4 getModelMatrix()     const { return modelMatrix; }

    // Alias kept for backwards compatibility — returns the same world matrix.
    [[nodiscard]] SMatrix4 getTransformMatrix() const { return modelMatrix; }

    // ── Transform setters ─────────────────────────────────────────────────────
    void setRelativePosition(const SVector3& position)
    {
        relativePosition = position;
        updateTransforms();
    }

    void setRelativeRotation(const SQuaternion& rotation)
    {
        relativeRotation = rotation;
        updateTransforms();
    }

    void setRelativeScale(const SVector3& scale)
    {
        relativeScale = scale;
        updateTransforms();
    }

    void setWorldPosition(const SVector3& worldPosition);
    void setWorldRotation(const SQuaternion& worldRotation);

    // ── Direction helpers ─────────────────────────────────────────────────────
    [[nodiscard]] SVector3 getForwardVector() const;
    [[nodiscard]] SVector3 getRightVector()   const;
    [[nodiscard]] SVector3 getUpVector()      const;

    // ── Hierarchy ─────────────────────────────────────────────────────────────
    [[nodiscard]] MSpatialEntity*              getParent()   { return parent; }
    [[nodiscard]] std::vector<MSpatialEntity*>& getChildren() { return children; }

    void setParentScene(MScene* scene);

    // Reparent this entity. Pass nullptr to detach and promote to scene root.
    void setParent(MSpatialEntity* newParent);

    void addChild(MSpatialEntity* entity);
    void removeChild(MSpatialEntity* entity);

    // ── Recursive search ──────────────────────────────────────────────────────
    template<typename T>
    T* find(const SString& searchName)
    {
        static_assert(std::is_base_of<MSpatialEntity, T>::value,
                      "T must derive from MSpatialEntity");

        if (name == searchName)
        {
            T* typed = dynamic_cast<T*>(this);
            if (typed) return typed;
        }

        for (auto* child : children)
        {
            if (!child) continue;
            if (T* result = child->find<T>(searchName))
                return result;
        }

        return nullptr;
    }

    // ── Update ────────────────────────────────────────────────────────────────
    void updateTransforms();

    // ── Overridable callbacks ─────────────────────────────────────────────────
    virtual void onCreate();
    virtual void onStart();
    virtual void onUpdate(float deltaTime);
    virtual void onExit();
    virtual void onDrawGizmo(SVector2 renderResolution);

    [[nodiscard]] bool hasStarted() const {return entityStarted; }
    [[nodiscard]] bool getCanTick() const { return canTick; }
    void setCanTick(bool canTick) { this->canTick = canTick; }

    // Subclasses may override to provide a display name for auto-naming.
    [[nodiscard]] virtual SString typeName() const { return STR("spatial"); }

    // ── Destructor ────────────────────────────────────────────────────────────
    ~MSpatialEntity() override = default;
private:
// Returns the local TRS matrix (does NOT include parent transforms).
    [[nodiscard]] SMatrix4 computeLocalMatrix() const;
private:
    static SString generateName(const SString& base);

protected:
    // Protected: use createInstance() from external code.
    // Subclasses may call these in their own factory overrides.
    MSpatialEntity();
    explicit MSpatialEntity(MSpatialEntity* parent);

    SVector3    relativePosition = SVector3(0.0f);
    SQuaternion relativeRotation = glm::identity<SQuaternion>();
    SVector3    relativeScale    = SVector3(1.0f);

    // Cached world-space matrix — kept current by updateTransforms().
    SMatrix4 modelMatrix = glm::identity<SMatrix4>();

    MSpatialEntity*              parent   = nullptr;
    std::vector<MSpatialEntity*> children;
    bool         enabled = true;
    EEntityFlags flags   = EEntityFlags::Default;

    MScene* ownerScene = nullptr;

private:
    bool canTick = false;
    bool entityStarted = false;
};

#endif //SPATIAL_H