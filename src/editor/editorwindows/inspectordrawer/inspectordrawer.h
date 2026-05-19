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
    DEFINE_OBJECT_SUBCLASS(MInspectorDrawer)
public:
    static void initialise();
    static void registerDrawer(MInspectorDrawer* drawer);
    static MInspectorDrawer* getDrawer(MSpatialEntity* entity);

    // Entry point called by the inspector window.
    // Calls onDrawInspector, then -- only when no custom drawer matched --
    // calls drawDefaultFields as a fallback for DECLARE_FIELD members.
    void onDraw(MSpatialEntity* target);

    virtual SString getInspectorName() const = 0;
    virtual bool    canDraw(MSpatialEntity* entity) = 0;

protected:
    virtual void onDrawInspector(MSpatialEntity* target) = 0;

    // Fallback field drawing. Only invoked by onDraw when the default drawer
    // is handling the entity (i.e. no custom drawer matched). Override in
    // the default drawer to iterate DECLARE_FIELD members. Custom drawers
    // never need to touch this.
    virtual void drawDefaultFields(MSpatialEntity*) {}

private:
    static std::vector<MInspectorDrawer*> drawers;
    static MInspectorDrawer*              defaultDrawer;
};

#endif //METEOR_ENGINE_INSPECTORDRAWER_H