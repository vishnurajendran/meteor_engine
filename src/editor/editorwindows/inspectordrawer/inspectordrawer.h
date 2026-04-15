//
// Created by Vishnu Rajendran on 2024-09-28.
//
#pragma once
#ifndef METEOR_ENGINE_INSPECTORDRAWER_H
#define METEOR_ENGINE_INSPECTORDRAWER_H

#include <vector>
#include "core/object/object.h"

class MSpatialEntity;

class MInspectorDrawer : public MObject {
public:
    static void initialise();
    static void registerDrawer(MInspectorDrawer* drawer);
    static MInspectorDrawer* getDrawer(MSpatialEntity* entity);

    // Entry point called by the inspector window
    void onDraw(MSpatialEntity* target);

    virtual SString getInspectorName() const = 0;
    virtual bool    canDraw(MSpatialEntity* entity) = 0;

protected:
    virtual void onDrawInspector(MSpatialEntity* target) = 0;

private:
    static std::vector<MInspectorDrawer*> drawers;
    static MInspectorDrawer*              defaultDrawer;
};

#endif //METEOR_ENGINE_INSPECTORDRAWER_H