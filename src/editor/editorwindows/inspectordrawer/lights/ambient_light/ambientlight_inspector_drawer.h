//
// Created by ssj5v on 24-04-2025.
//
#pragma once
#ifndef AMBIENTLIGHT_INSPECTOR_DRAWER_H
#define AMBIENTLIGHT_INSPECTOR_DRAWER_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MAmbientLightEntity;

class MAmbientLightInspectorDrawer : public MSpatialEntityInspectorDrawer {
public:
    SString getInspectorName() const override { return "Ambient Light"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    void drawALGui(MAmbientLightEntity* light);
    static const bool registered;
};

#endif //AMBIENTLIGHT_INSPECTOR_DRAWER_H