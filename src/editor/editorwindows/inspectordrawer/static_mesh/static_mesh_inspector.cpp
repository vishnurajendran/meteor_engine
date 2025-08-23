//
// Created by ssj5v on 16-05-2025.
//

#include "static_mesh_inspector.h"

#include "core/engine/3d/material/MMaterialAsset.h"
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/editorwindows/inspectordrawer/controls/material_properties_controls.h"

bool MStaticMeshInspectorDrawer::registered = []()
{
    registerDrawer(new MStaticMeshInspectorDrawer());
    return true;
}();

MStaticMeshInspectorDrawer::MStaticMeshInspectorDrawer()
{
    staticMeshAssetReferenceControl = new MAssetReferenceControl();
    staticMeshAssetReferenceControl->canAcceptAssetFuncCallback = [this](MAsset* asset)
    { return dynamic_cast<MStaticMeshAsset*>(asset) != nullptr; };

    materialAssetReferenceControl = new MAssetReferenceControl();
    materialAssetReferenceControl->canAcceptAssetFuncCallback = [this](MAsset* asset)
    { return dynamic_cast<MMaterialAsset*>(asset) != nullptr; };

    materialPropertyControl = new MMaterialPropertyControl();
}

bool MStaticMeshInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MStaticMeshEntity*>(entity) != nullptr;
}

void MStaticMeshInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    auto staticMeshTarget = dynamic_cast<MStaticMeshEntity*>(target);

    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    auto assetRef = staticMeshAssetReferenceControl->getAssetReference();
    auto targetAssetRef = staticMeshTarget->getStaticMeshAsset();

    auto materialAssetRef = materialAssetReferenceControl->getAssetReference();
    auto materialTargetAssetRef = staticMeshTarget->getMaterialAsset();

    if (assetRef != targetAssetRef)
    {
        staticMeshAssetReferenceControl->setAssetReference(targetAssetRef);
    }

    if (materialAssetRef != materialTargetAssetRef)
    {
        materialAssetReferenceControl->setAssetReference(materialTargetAssetRef);
    }

    if (ImGui::CollapsingHeader("Static Mesh Entity"))
    {
        if (staticMeshAssetReferenceControl->drawControl("Static Mesh Asset"))
        {
            staticMeshTarget->setStaticMeshAsset(dynamic_cast<MStaticMeshAsset*>(staticMeshAssetReferenceControl->getAssetReference()));
        }

        if (materialAssetReferenceControl->drawControl("Material Asset"))
        {
            staticMeshTarget->setMaterialAsset(dynamic_cast<MMaterialAsset*>(materialAssetReferenceControl->getAssetReference()));
        }
    }

    materialPropertyControl->draw(staticMeshTarget->getMaterialInstance());
}
