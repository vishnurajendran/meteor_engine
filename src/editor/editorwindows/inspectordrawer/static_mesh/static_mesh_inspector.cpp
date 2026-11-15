#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "static_mesh_inspector.h"

#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/memory/mnew.h"

bool MStaticMeshInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MStaticMeshInspectorDrawer>());
    return true;
}();

MStaticMeshInspectorDrawer::MStaticMeshInspectorDrawer()
{
    meshAssetControl = mnew<MAssetReferenceControl>();
    meshAssetControl->canAcceptAssetFuncCallback = [](const TAssetHandle<MAsset>& asset)
    { return dynamic_cast<MStaticMeshAsset*>(asset.get()) != nullptr; };
}

bool MStaticMeshInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MStaticMeshEntity*>(entity) != nullptr;
}

void MStaticMeshInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    auto* sme = dynamic_cast<MStaticMeshEntity*>(target);
    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    if (ImGui::CollapsingHeader("Static Mesh", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const auto cur        = sme->getStaticMeshAsset();
        const auto controlRef = meshAssetControl->getAssetReference();

        if (controlRef.getAssetId() != cur.getAssetId())
            meshAssetControl->setAssetReference(cur);

        START_FIELD_DRAW("Mesh")
            if (meshAssetControl->drawControl(""))
            {
                TAssetHandle<MStaticMeshAsset> meshRef = meshAssetControl->getAssetReference();
                sme->setStaticMeshAsset(meshRef);
            }
        END_FIELD_DRAW()

        START_FIELD_DRAW("Cast Shadow")
            bool cs = sme->getCastsShadow();
            if (ImGui::Checkbox("##v", &cs)) sme->setCastsShadow(cs);
        END_FIELD_DRAW()

        START_FIELD_DRAW("Show Mesh Bounds")
            bool gizmosEnabled = sme->getDrawGizmo();
            if (ImGui::Checkbox("##v", &gizmosEnabled)) sme->setDrawGizmo(gizmosEnabled);
        END_FIELD_DRAW()
    }

    materialSlots.draw(sme);
}
