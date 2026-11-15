#pragma once
#ifndef STATIC_MESH_INSPECTOR_H
#define STATIC_MESH_INSPECTOR_H

#include "editor/editorwindows/inspectordrawer/mesh_common/material_slots_drawer.h"
#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MAssetReferenceControl;

class MStaticMeshInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MStaticMeshInspectorDrawer)
public:
    SString getInspectorName() const override { return "Static Mesh"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    MStaticMeshInspectorDrawer();

    MAssetReferenceControl* meshAssetControl;
    // Material properties are edited by selecting the material asset directly.
    MMaterialSlotsDrawer    materialSlots;

    static bool registered;
};

#endif
