//
// Created by ssj5v on 01-05-2025.
//
#pragma once
#ifndef SPOT_LIGHT_INSPECTOR_DRAWER_H
#define SPOT_LIGHT_INSPECTOR_DRAWER_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MSpotLightInspectorDrawer : public MSpatialEntityInspectorDrawer {
public:
    SString getInspectorName() const override { return "Spot Light"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    void drawSLGui(MSpatialEntity* target);
    static bool registered;
};

#endif //SPOT_LIGHT_INSPECTOR_DRAWER_H