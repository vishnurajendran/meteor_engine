//
// MSkyboxInspectorDrawer
// Inspector panel for MSkyboxEntity - shows a cubemap asset reference control.
//

#pragma once
#ifndef SKYBOX_INSPECTOR_DRAWER_H
#define SKYBOX_INSPECTOR_DRAWER_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MAssetReferenceControl;

class MSkyboxInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MSkyboxInspectorDrawer)
public:
    SString getInspectorName() const override { return "Skybox"; }
    bool    canDraw(MSpatialEntity* entity)   override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    MSkyboxInspectorDrawer();

    MAssetReferenceControl* cubemapAssetControl = nullptr;

    static bool registered;
};

#endif // SKYBOX_INSPECTOR_DRAWER_H