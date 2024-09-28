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
    MSpatialEntity(MSpatialEntity *parent);
    ~MSpatialEntity();

    SVector3 getRelativePosition() { return relativePosition; }
    SVector3 getRelativeScale() { return relativeScale; };
    SQuaternion getRelativeRotation() { return relativeRotation; }

    void setRelativePosition(SVector3 position) { relativePosition = position; }
    void setRelativeRotation(SQuaternion localRotation) { this->relativeRotation = localRotation; }
    void setRelativeScale(SVector3 scale) { relativeScale = scale;}

    MSpatialEntity *getParent() { return parent; }
    std::vector<MSpatialEntity *>* getChildren() { return children; }

    void addChild(MSpatialEntity *entity);
    void removeChild(MSpatialEntity *entity);

    template<typename T>
    T *find(SString name) {
        static_assert(std::is_base_of<MSpatialEntity, T>::value, "T must inherit from MSpatialEntity");
        // is this node called 'name'
        if (this->name == name)
            return (T *) this;

        // if no more children return nullptr
        if (children->size() <= 0)
            return nullptr;

        // check recursively for each child
        for (auto child: *children) {
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

    virtual void onStart();
    virtual void onUpdate(float deltaTime);
    virtual void onExit();

protected:
    SVector3 relativePosition = SVector3(0);
    SVector3 relativeScale = SVector3(1);
    SQuaternion relativeRotation = glm::identity<SQuaternion>();

    MSpatialEntity *parent = nullptr;
    std::vector<MSpatialEntity *> *children = nullptr;

    void updateChildren(float deltaTime);

private:
    void setParent(MSpatialEntity *entity) { parent = entity; };
};

#endif //SPATIAL_H
