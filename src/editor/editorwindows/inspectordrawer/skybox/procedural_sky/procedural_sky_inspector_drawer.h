//
// MProceduralSkyInspectorDrawer
// Inspector panel for MProceduralSkyboxEntity — mirrors the layout and
// registration pattern of MStaticMeshInspectorDrawer.
//

#pragma once
#ifndef PROCEDURAL_SKY_INSPECTOR_H
#define PROCEDURAL_SKY_INSPECTOR_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MProceduralSkyInspectorDrawer : public MSpatialEntityInspectorDrawer
{
public:
    SString getInspectorName() const override { return "Procedural Skybox"; }
    bool    canDraw(MSpatialEntity* entity)   override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    MProceduralSkyInspectorDrawer() = default;
    static bool registered;
};

#endif // PROCEDURAL_SKY_INSPECTOR_H