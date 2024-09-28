//
// Created by Vishnu Rajendran on 2024-09-28.
//
#pragma once
#ifndef METEOR_ENGINE_INSPECTORDRAWER_H
#define METEOR_ENGINE_INSPECTORDRAWER_H

#include <typeinfo>
#include <map>
#include "core/object/object.h"

class MSpatialEntity;

class MInspectorDrawer : public MObject {
private:
    static std::map<const std::type_info*, MInspectorDrawer*>* drawers;
    static MInspectorDrawer* defaultDrawer;
public:
    static void initialise();
    static void registerDrawer(MInspectorDrawer* drawer);
    virtual void onDrawInspector(MSpatialEntity* target)=0;
    static MInspectorDrawer* getDrawer(MSpatialEntity* entity);
};

#endif //METEOR_ENGINE_INSPECTORDRAWER_H
