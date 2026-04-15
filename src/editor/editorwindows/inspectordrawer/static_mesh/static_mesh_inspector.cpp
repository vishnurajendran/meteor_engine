//
// Created by ssj5v on 16-05-2025.
//

#include "static_mesh_inspector.h"

#include "core/graphics/core/material/MMaterialAsset.h"
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
        constexpr float LABEL_COL_W = 120.0f;
        if (ImGui::BeginTable("##sm_refs", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("label",  ImGuiTableColumnFlags_WidthFixed,   LABEL_COL_W);
            ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

            // ── Static Mesh Source ──────────────────────────────────────────
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Static Mesh:");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (staticMeshAssetReferenceControl->drawControl(""))
            {
                staticMeshTarget->setStaticMeshAsset(
                    dynamic_cast<MStaticMeshAsset*>(staticMeshAssetReferenceControl->getAssetReference()));
            }

            // ── Material ────────────────────────────────────────────────────
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Material Src:");

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (materialAssetReferenceControl->drawControl(""))
            {
                staticMeshTarget->setMaterialAsset(
                    dynamic_cast<MMaterialAsset*>(materialAssetReferenceControl->getAssetReference()));
            }

            ImGui::EndTable();
        }
    }

    materialPropertyControl->draw(staticMeshTarget->getMaterialInstance());
}