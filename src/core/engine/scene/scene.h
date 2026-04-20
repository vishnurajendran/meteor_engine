//
// Created by ssj5v on 26-09-2024.
//
#pragma once
#ifndef SCENE_H
#define SCENE_H

#include <core/meteor_core_minimal.h>
#include <pugixml.hpp>
#include <unordered_set>


class MSpatialEntity;
class MScene : public MObject {
    friend class MSpatialEntity;
public:

    MScene();
    ~MScene() override;
    void startScene() const;
    void update(float deltaTime);
    void close();
    void addToRoot(MSpatialEntity* entity);
    size_t getRootSize() { return rootEntities.size(); }
    std::vector<MSpatialEntity*>& getRootEntities() { return rootEntities; }
    bool tryParse(pugi::xml_document* doc);
    bool isClosing() { return sceneClosing; }
    template<typename T>
    T* find(SString name) {
        if (rootEntities.size() <= 0)
            return NULL;

        for (auto rootEntity : rootEntities) {
            auto res = rootEntity->template find<T>(name);
            if (res != NULL)
                return res;
        }

        return NULL;
    }

private:
    std::vector<MSpatialEntity*> rootEntities;
    std::unordered_map<MObject*, MObjectPtr<MSpatialEntity>> allAliveEntities;
    std::unordered_set<MSpatialEntity*> markedForDestroy;

    void recursivelyLoadEntity(pugi::xml_node currNode, MSpatialEntity* parent);
    bool sceneClosing = false;
public:
    std::unordered_map<MObject*, MObjectPtr<MSpatialEntity>>& getAllEntities() { return allAliveEntities; }
    bool registerEntity(MSpatialEntity* entity);
    void markForDestroy(MSpatialEntity* entity);
    void destroyMarked();
public:
    static const SString VALID_SCENE_FILE_XML_TAG;
    static const SString SCENE_NAME_TAG;
};

#endif //SCENE_H
