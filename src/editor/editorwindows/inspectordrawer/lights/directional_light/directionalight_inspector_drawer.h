//
// Created by ssj5v on 24-04-2025.
//
#pragma once
#ifndef DIRECTIONALIGHT_INSPECTOR_DRAWER_H
#define DIRECTIONALIGHT_INSPECTOR_DRAWER_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MDirectionalLight;

// BUG FIX: was `class MDirecionalLightInspectorDrawer : MSpatialEntityInspectorDrawer`
// (private inheritance), which silently broke registerDrawer() access.
class MDirecionalLightInspectorDrawer : public MSpatialEntityInspectorDrawer {
public:
    SString getInspectorName() const override { return "Directional Light"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    void drawDLGui(MDirectionalLight* light);
    static const bool registered;
};

#endif //DIRECTIONALIGHT_INSPECTOR_DRAWER_H