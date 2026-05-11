//
// spatial.h
//

#pragma once
#ifndef SPATIAL_H
#define SPATIAL_H

#include <core/meteor_core_minimal.h>
#include <vector>
#include "core/engine/entities/entity_type_registry.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "data/serialized_class_base.h"
#include "entityflags.h"
#include "spatial_class_macros.h"
#include "core/utils/field_engine_types.h"

/// Spatial entity is the 3D object that populates the scene.
/// Every object that lives within the scene needs to inherit this class.\n\n
/// [Important]\n ensure all inherited classes for spatial classes use\n
/// "DEFINE_SPATIAL_CLASS" macro in header and "IMPLEMENT_SPATIAL_CLASS" macro in the source file.
/// These macros auto-registers the class for serialisation within the scene.
class MSpatialEntity : public MObject, public SerializedClassBase
{
    DEFINE_SPATIAL_CLASS(MSpatialEntity)

public:
    /// Function for creating default spatial entity instances
    static MSpatialEntity* createInstance(const SString& name = {});

    /// Templated function for creating spatial entity instances.
    template<typename T>
    static T* createInstance(const SString& name, MScene* ownerScene = nullptr)
    {
        static_assert(std::is_base_of<MSpatialEntity, T>::value,
                      "T must derive from MSpatialEntity");
        T* entity = new T();
        entity->setName(name);
        if (!ownerScene)
        {
            ownerScene = MSceneManager::getSceneManagerInstance()->getActiveScene();
            if (!ownerScene) MERROR("NO SCENE OPEN");
        }
        ownerScene->registerEntity(entity);
        entity->ownerScene = ownerScene;
        return entity;
    }

    // Scene serialization
    pugi::xml_node serialiseEntity(pugi::xml_node parent) const;
    static MSpatialEntity* deserialiseEntity(const pugi::xml_node& node);

    /// Mark this object for destruction.\n\n
    /// Object marked are destroyed during the next frame before onTick() is invoked.
    void destroy();

    // Flags / enable
    [[nodiscard]] EEntityFlags getEntityFlags() const { return flags; }
    void setEntityFlags(EEntityFlags flag)             { flags = flag; }

    void setEnabled(bool enable)          { enabled = enable; }
    [[nodiscard]] bool getEnabled() const { return enabled; }

    // Transform getters
    [[nodiscard]] SVector3    getRelativePosition() const { return relativePosition; }
    [[nodiscard]] SVector3    getRelativeScale()    const { return relativeScale; }
    [[nodiscard]] SQuaternion getRelativeRotation() const { return relativeRotation; }

    [[nodiscard]] SVector3    getWorldPosition() const;
    [[nodiscard]] SQuaternion getWorldRotation() const;
    [[nodiscard]] SMatrix4    getModelMatrix()     const { return modelMatrix; }
    [[nodiscard]] SMatrix4    getTransformMatrix() const { return modelMatrix; }

    // Transform setters
    void setRelativePosition(const SVector3& p)    { relativePosition = p; updateTransforms(); }
    void setRelativeRotation(const SQuaternion& r) { relativeRotation = r; updateTransforms(); }
    void setRelativeScale(const SVector3& s)       { relativeScale = s;    updateTransforms(); }

    void setWorldPosition(const SVector3& worldPosition);
    void setWorldRotation(const SQuaternion& worldRotation);

    /// Get the Forward vector
    [[nodiscard]] SVector3 getForwardVector() const;
    /// Get the Right vector
    [[nodiscard]] SVector3 getRightVector()   const;
    /// Get the Up vector
    [[nodiscard]] SVector3 getUpVector()      const;

    [[nodiscard]] MSpatialEntity*               getParent()   { return parent; }
    [[nodiscard]] std::vector<MSpatialEntity*>& getChildren() { return children; }

    void setParentScene(MScene* scene);

    /// Set the parent for this object
    void setParent(MSpatialEntity* newParent);
    /// Add a child to this object
    void addChild(MSpatialEntity* entity);
    /// Removes a child from this entity.
    void removeChild(MSpatialEntity* entity);

    /// Templated function to recursively search an entity within this entity.\n\n
    /// **The name must match.**
    template<typename T>
    T* find(const SString& searchName)
    {
        static_assert(std::is_base_of<MSpatialEntity, T>::value,
                      "T must derive from MSpatialEntity");
        if (name == searchName)
            if (T* typed = dynamic_cast<T*>(this)) return typed;
        for (auto* child : children)
            if (child)
                if (T* result = child->find<T>(searchName)) return result;
        return nullptr;
    }

    void updateTransforms();

    /// Called when instance is created.
    virtual void onCreate();
    /// Called after onCreate, used for initialisations.
    virtual void onStart();

    /// Called every frame.
    virtual void onUpdate(float deltaTime);

    /// Called before destroying the object.
    virtual void onExit();

    /// Called by the editor when drawing gizmos, override this function
    /// to draw custom Gizmo for this object.
    virtual void onDrawGizmo(SVector2 renderResolution);

    /// Returns true if the object called and completed onStart().
    [[nodiscard]] bool hasStarted() const { return entityStarted; }
    [[nodiscard]] bool getCanTick() const { return canTick; }

    /// Decides if this object can tick, default value is True
    void setCanTick(bool tick)            { canTick = tick; }

    ~MSpatialEntity() override = default;

protected:
    void onSerialise(pugi::xml_node& node)         override;
    void onDeserialise(const pugi::xml_node& node) override;

    MSpatialEntity();
    explicit MSpatialEntity(MSpatialEntity* parent);

    SVector3    relativePosition = SVector3(0.0f);
    SQuaternion relativeRotation = glm::identity<SQuaternion>();
    SVector3    relativeScale    = SVector3(1.0f);
    SMatrix4    modelMatrix      = glm::identity<SMatrix4>();

    MSpatialEntity*              parent    = nullptr;
    std::vector<MSpatialEntity*> children;
    bool         enabled    = true;
    EEntityFlags flags      = EEntityFlags::Default;
    MScene*      ownerScene = nullptr;

private:
    [[nodiscard]] SMatrix4 computeLocalMatrix() const;
    static SString generateName(const SString& base);

    bool canTick       = false;
    bool entityStarted = false;
};
#endif // SPATIAL_H