//
// Created by ssj5v on 26-09-2024.
//
#pragma once
#ifndef SCENE_H
#define SCENE_H

#include <pugixml.hpp>
#include <unordered_set>

#include "core/engine/entities/spatial/spatial.h"
#include "core/object/object.h"
#include "core/object/objectpointer.h"


SCRIPT_BIND_CLASS()
class MScene : public MObject {
    friend class MSpatialEntity;
public:

    MScene();
    ~MScene() override;
    // End of the scene build - awakes every loaded entity (see onAwake).
    void startScene();
    void update(float deltaTime);
    void fixedUpdate(float fixedDeltaTime);
    void lateUpdate(float deltaTime);
    // Physics hooks, driven by the application's physics loop:
    // postPhysicsStep after every step, interpolatePhysics once a frame
    // after the last step (alpha = how far between the last two steps).
    void postPhysicsStep(float fixedDeltaTime);
    void interpolatePhysics(float alpha);
    void close();
    void addToRoot(MSpatialEntity* entity);

    SCRIPT_BIND_FUNC()
    size_t getRootSize() { return rootEntities.size(); }

    SCRIPT_BIND_FUNC()
    std::vector<MSpatialEntity*>& getRootEntities() { return rootEntities; }

    SCRIPT_BIND_FUNC()
    bool isClosing() { return sceneClosing; }

    bool tryParse(pugi::xml_document* doc);

    // Removes entity from its current parent/root, inserts into root list at index.
    void insertRootEntityAt(MSpatialEntity* entity, int index);

    SCRIPT_BIND_FUNC()
    MSpatialEntity* find(const SString& path);

    template<typename T>
    T* findT(SString name) {
        if (rootEntities.size() <= 0)
            return NULL;

        for (auto rootEntity : rootEntities) {
            auto res = rootEntity->template findT<T>(name);
            if (res != NULL)
                return res;
        }

        return NULL;
    }

private:
    std::vector<MSpatialEntity*> rootEntities;
    std::unordered_map<MObject*, MObjectPtr<MSpatialEntity>> allAliveEntities;
    std::unordered_set<MSpatialEntity*> markedForDestroy;

    // Every entity reachable from the roots, parents before children, in
    // hierarchy order. The scene passes walk this instead of the map, so
    // update order is stable. Rebuilt lazily when the hierarchy changes.
    std::vector<MSpatialEntity*> entityOrder;
    bool entityOrderDirty = true;
    const std::vector<MSpatialEntity*>& getEntityOrder();

    // Registered but not yet awoken, in registration order (a scene build
    // registers parents before children). Drained by awakePending().
    std::vector<MSpatialEntity*> pendingAwake;
    void awakePending();

    void recursivelyLoadEntity(pugi::xml_node currNode, MSpatialEntity* parent);
    bool sceneClosing = false;
public:
    std::unordered_map<MObject*, MObjectPtr<MSpatialEntity>>& getAllEntities() { return allAliveEntities; }
    bool registerEntity(MSpatialEntity* entity);
    // Call after changing the root list or any entity's children, so the
    // next scene pass rebuilds its walk order.
    void markHierarchyDirty() { entityOrderDirty = true; }
    void markForDestroy(MSpatialEntity* entity);
    void destroyMarked();
public:
    static const SString VALID_SCENE_FILE_XML_TAG;
    static const SString SCENE_NAME_TAG;
};

#endif //SCENE_H