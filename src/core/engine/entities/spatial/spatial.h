//
// Created by ssj5v on 26-09-2024.
//
#pragma once
#ifndef SPATIAL_H
#define SPATIAL_H
#include <core/meteor_core_minimal.h>
#include <vector>

#include "entityflags.h"

class MSpatialEntity : public MObject
{
public:
    MSpatialEntity();
    explicit MSpatialEntity(MSpatialEntity* parent);
    ~MSpatialEntity() override;

    [[nodiscard]] EEntityFlags getEntityFlags() const { return flags; }
    void setEntityFlags(EEntityFlags flag) { this->flags = flag; }

    void setEnabled(bool enable) { this->enabled = enable; }
    [[nodiscard]] bool getEnabled() const { return this->enabled; }

    [[nodiscard]] SVector3 getRelativePosition() const { return relativePosition; }
    [[nodiscard]] SVector3 getWorldPosition() const;

    [[nodiscard]] SVector3 getRelativeScale() const { return relativeScale; };

    [[nodiscard]] SQuaternion getRelativeRotation() const { return relativeRotation; }
    [[nodiscard]] SQuaternion getWorldRotation() const;

    [[nodiscard]] SMatrix4 getModelMatrix() const;

    void setRelativePosition(const SVector3& position)
    {
        relativePosition = position;
        updateTransforms();
    }
    void setWorldPosition(const SVector3& worldPosition);

    void setRelativeRotation(const SQuaternion& localRotation)
    {
        this->relativeRotation = localRotation;
        updateTransforms();
    }
    void setWorldRotation(const SQuaternion& worldRotation);

    void setRelativeScale(const SVector3& scale)
    {
        relativeScale = scale;
        updateTransforms();
    }

    MSpatialEntity* getParent() { return parent; }
    std::vector<MSpatialEntity*>& getChildren() { return children; }

    void addChild(MSpatialEntity* entity);
    void removeChild(MSpatialEntity* entity);

    SMatrix4 getTransformMatrix() const;
    SVector3 getForwardVector() const;
    SVector3 getRightVector() const;
    SVector3 getUpVector() const;

    template <typename T>
    T* find(SString name)
    {
        static_assert(std::is_base_of<MSpatialEntity, T>::value, "T must inherit from MSpatialEntity");
        // is this node called 'name'
        if (this->name == name)
            return (T*)this;

        // if no more children return nullptr
        if (children.size() <= 0)
            return nullptr;

        // check recursively for each child
        for (auto child : children)
        {
            if (child == nullptr)
                continue;

            auto res = child->find<T>(name);
            if (res == nullptr)
                continue;

            auto isValidInstance = instanceof<T>((T*)res);
            // if we found it, return
            if (res != nullptr && isValidInstance)
                return (T*)res;
        }

        // if everything fails, return nullptr
        return nullptr;
    }

    void updateTransforms();
    virtual void onStart();
    virtual void onUpdate(float deltaTime);
    virtual void onExit();
    virtual void onDrawGizmo();

private:
    void setParent(MSpatialEntity* entity) { parent = entity; };

protected:
    SVector3 relativePosition = SVector3(0);
    SQuaternion relativeRotation = glm::identity<SQuaternion>();
    SVector3 relativeScale = SVector3(1);

    SMatrix4 modelMatrix = glm::identity<SMatrix4>();

    MSpatialEntity* parent = nullptr;
    std::vector<MSpatialEntity*> children;
    bool enabled = true;
    EEntityFlags flags = EEntityFlags::Default;

public:
    static void updateAllSceneEntities(float deltaTime);
    static void destroy(MSpatialEntity* entity);

private:
    static std::map<SString, MSpatialEntity*> allAliveEntities;
    static void addAliveEntity(MSpatialEntity* entity);
    static void removeAliveEntity(MSpatialEntity* entity);

    static std::vector<MSpatialEntity*> markedForDestruction;
public:
    static void destroyMarked();
};


#endif //SPATIAL_H
