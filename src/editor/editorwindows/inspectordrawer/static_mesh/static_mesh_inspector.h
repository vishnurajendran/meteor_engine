#pragma once
#ifndef STATIC_MESH_INSPECTOR_H
#define STATIC_MESH_INSPECTOR_H

#include "core/graphics/core/material/MMaterialAsset.h"
#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MAssetReferenceControl;

class MStaticMeshInspectorDrawer : public MSpatialEntityInspectorDrawer
{
public:
    SString getInspectorName() const override { return "Static Mesh"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    MStaticMeshInspectorDrawer();

    // One reference control per material slot — no property panel here.
    // Material properties are edited by selecting the material asset directly.
    struct SSlotControls
    {
        MAssetReferenceControl* assetRef       = nullptr;
        MMaterialAsset*         lastKnownAsset = nullptr;
    };

    void ensureSlotControls(int count);

    MAssetReferenceControl*    meshAssetControl;
    std::vector<SSlotControls> slotControls;

    static bool registered;
};

#endif