//
// Created by Vishnu Rajendran on 2024-09-28.
//
#pragma once
#ifndef METEOR_ENGINE_INSPECTORDRAWER_H
#define METEOR_ENGINE_INSPECTORDRAWER_H

#include <typeinfo>
#include <map>
#include <vector>

#include "core/object/object.h"

class MSpatialEntity;

class MInspectorDrawer : public MObject {
private:
    static std::vector<MInspectorDrawer*> drawers;
    static MInspectorDrawer* defaultDrawer;
public:
    static void initialise();
    static void registerDrawer(MInspectorDrawer* drawer);
    virtual SString getInspectorName() const=0;
    void onDraw(MSpatialEntity* target);
    virtual bool canDraw(MSpatialEntity* entity) = 0;
    static MInspectorDrawer* getDrawer(MSpatialEntity* entity);
protected:
    virtual void onDrawInspector(MSpatialEntity* target)=0;
};

#endif //METEOR_ENGINE_INSPECTORDRAWER_H
