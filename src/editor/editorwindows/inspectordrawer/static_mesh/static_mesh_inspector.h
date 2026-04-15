//
// Created by ssj5v on 16-05-2025.
//
#pragma once
#ifndef STATIC_MESH_INSPECTOR_H
#define STATIC_MESH_INSPECTOR_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MMaterialPropertyControl;
class MAssetReferenceControl;

class MStaticMeshInspectorDrawer : public MSpatialEntityInspectorDrawer {
public:
    SString getInspectorName() const override { return "Static Mesh"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    MStaticMeshInspectorDrawer();

    MAssetReferenceControl*  staticMeshAssetReferenceControl;
    MAssetReferenceControl*  materialAssetReferenceControl;
    MMaterialPropertyControl* materialPropertyControl;

    static bool registered;
};

#endif //STATIC_MESH_INSPECTOR_H