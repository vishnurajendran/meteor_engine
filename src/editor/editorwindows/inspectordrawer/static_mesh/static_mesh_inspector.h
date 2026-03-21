//
// Created by ssj5v on 16-05-2025.
//

#ifndef STATIC_MESH_INSPECTOR_H
#define STATIC_MESH_INSPECTOR_H
#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"


class MMaterialPropertyControl;
class MAssetReferenceControl;
class MStaticMeshInspectorDrawer : public MSpatialEntityInspectorDrawer {
private:
    MAssetReferenceControl* staticMeshAssetReferenceControl;
    MAssetReferenceControl* materialAssetReferenceControl;
    MMaterialPropertyControl* materialPropertyControl;
    MStaticMeshInspectorDrawer();
public:
    bool canDraw(MSpatialEntity* entity) override;
    void onDrawInspector(MSpatialEntity* target) override;
private:
    static bool registered;
};



#endif //STATIC_MESH_INSPECTOR_H
