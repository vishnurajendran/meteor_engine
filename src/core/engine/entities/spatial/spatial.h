//
// Created by ssj5v on 26-09-2024.
//
#pragma once
#ifndef SPATIAL_H
#define SPATIAL_H
#include <core/meteor_core_minimal.h>
#include <vector>

class MSpatialEntity : public MObject {
public:
    MSpatialEntity() : MSpatialEntity(nullptr) {}
    explicit MSpatialEntity(MSpatialEntity *parent);
    ~MSpatialEntity() override;

    void setEnabled(bool enable) {this->enabled = enable;}
    [[nodiscard]] bool getEnabled() const {return this->enabled;}

    [[nodiscard]] SVector3 getRelativePosition() const { return relativePosition; }
    [[nodiscard]] SVector3 getWorldPosition() const ;

    [[nodiscard]] SVector3 getRelativeScale() const { return relativeScale; };

    [[nodiscard]] SQuaternion getRelativeRotation() const { return relativeRotation; }
    [[nodiscard]] SQuaternion getWorldRotation() const ;

    [[nodiscard]] SMatrix4 getModelMatrix() const;

    void setRelativePosition(SVector3 position) { relativePosition = position; updateTransforms();}
    void setRelativeRotation(SQuaternion localRotation) { this->relativeRotation = localRotation; updateTransforms();}
    void setRelativeScale(SVector3 scale) { relativeScale = scale; updateTransforms();}

    MSpatialEntity *getParent() { return parent; }
    std::vector<MSpatialEntity *> getChildren() { return children; }

    void addChild(MSpatialEntity *entity);
    void removeChild(MSpatialEntity *entity);

    template<typename T>
    T *find(SString name) {
        static_assert(std::is_base_of<MSpatialEntity, T>::value, "T must inherit from MSpatialEntity");
        // is this node called 'name'
        if (this->name == name)
            return (T *) this;

        // if no more children return nullptr
        if (children.size() <= 0)
            return nullptr;

        // check recursively for each child
        for (auto child: children) {
            if (child == nullptr)
                continue;

            auto res = child->find<T>(name);
            if (res == nullptr)
                continue;

            auto isValidInstance = instanceof<T>((T *) res);
            //if we found it, return
            if (res != nullptr && isValidInstance)
                return (T *) res;
        }

        // if everything fails, return nullptr
        return nullptr;
    }

    void updateTransforms();
    virtual void onStart();
    virtual void onUpdate(float deltaTime);
    virtual void onExit();
private:
    void setParent(MSpatialEntity *entity) {
        parent = entity;
    };
protected:
    SVector3 relativePosition = SVector3(0);
    SQuaternion relativeRotation = glm::identity<SQuaternion>();
    SMatrix4 modelMatrix = glm::identity<SMatrix4>();
    SVector3 relativeScale = SVector3(1);
    MSpatialEntity *parent = nullptr;
    std::vector<MSpatialEntity*> children;
    bool enabled = true;
};

#endif //SPATIAL_H
