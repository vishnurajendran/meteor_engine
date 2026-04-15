//
// Created by ssj5v on 01-05-2025.
//
#pragma once
#ifndef POINT_LIGHT_INSPECTOR_DRAWER_H
#define POINT_LIGHT_INSPECTOR_DRAWER_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MPointLightInspectorDrawer : public MSpatialEntityInspectorDrawer {
public:
    SString getInspectorName() const override { return "Point Light"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    void drawPLGui(MSpatialEntity* target);
    static bool registered;
};

#endif //POINT_LIGHT_INSPECTOR_DRAWER_H