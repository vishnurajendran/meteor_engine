//
// Created by ssj5v on 26-09-2024.
//
#pragma once
#ifndef SCENE_H
#define SCENE_H

#include <core/meteor_core_minimal.h>
#include <core/engine/entities/spatial/spatial.h>
#include <pugixml.hpp>


class MScene : public MObject {
    friend class MSpatialEntity;
public:

    MScene();
    ~MScene();
    void startScene() const;
    void update(float deltaTime) const;
    void onClose();
    void addToRoot(MSpatialEntity* entity);
    size_t getRootSize() { return rootEntities.size(); }
    std::vector<MSpatialEntity*>& getRootEntities() { return rootEntities; };
    bool tryParse(pugi::xml_document* doc);
    bool isClosing() { return sceneClosing; }
    template<typename T>
    T* find(SString name) {
        if (rootEntities.size() <= 0)
            return NULL;

        for (auto rootEntity : rootEntities) {
            auto res = rootEntity->find<T>(name);
            if (res != NULL)
                return res;
        }

        return NULL;
    }

private:
    std::vector<MSpatialEntity*> rootEntities;
    void recursivelyLoadEntity(pugi::xml_node currNode, MSpatialEntity* parent);
    bool sceneClosing = false;
public:
    static const SString VALID_SCENE_FILE_XML_TAG;
    static const SString SCENE_NAME_TAG;
};

#endif //SCENE_H
