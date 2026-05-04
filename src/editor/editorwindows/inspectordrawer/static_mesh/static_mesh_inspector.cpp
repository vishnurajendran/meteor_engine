#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "static_mesh_inspector.h"

#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"

bool MStaticMeshInspectorDrawer::registered = []()
{
    registerDrawer(new MStaticMeshInspectorDrawer());
    return true;
}();

MStaticMeshInspectorDrawer::MStaticMeshInspectorDrawer()
{
    meshAssetControl = new MAssetReferenceControl();
    meshAssetControl->canAcceptAssetFuncCallback = [](MAsset* asset)
    { return dynamic_cast<MStaticMeshAsset*>(asset) != nullptr; };
}

bool MStaticMeshInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MStaticMeshEntity*>(entity) != nullptr;
}

void MStaticMeshInspectorDrawer::ensureSlotControls(int count)
{
    while ((int)slotControls.size() < count)
    {
        SSlotControls sc;
        sc.assetRef = new MAssetReferenceControl();
        sc.assetRef->canAcceptAssetFuncCallback = [](MAsset* asset)
        { return dynamic_cast<MMaterialAsset*>(asset) != nullptr; };
        slotControls.push_back(std::move(sc));
    }
}

void MStaticMeshInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    auto* sme = dynamic_cast<MStaticMeshEntity*>(target);
    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    // ── Mesh + cast shadow ────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Static Mesh", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 120.f;
        if (ImGui::BeginTable("##sm", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            auto* cur = sme->getStaticMeshAsset();
            if (meshAssetControl->getAssetReference() != cur)
                meshAssetControl->setAssetReference(cur);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mesh:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            if (meshAssetControl->drawControl(""))
                sme->setStaticMeshAsset(dynamic_cast<MStaticMeshAsset*>(
                                            meshAssetControl->getAssetReference()));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Cast Shadow:");
            ImGui::TableSetColumnIndex(1);
            bool cs = sme->getCastsShadow();
            if (ImGui::Checkbox("##cs", &cs)) sme->setCastsShadow(cs);

            ImGui::EndTable();
        }
    }

    // ── Material slots — references only ─────────────────────────────────────
    // Properties are edited by selecting the material asset in the asset browser.
    const int slotCount = sme->getMaterialSlotCount();
    ensureSlotControls(slotCount);

    for (int i = 0; i < slotCount; ++i)
    {
        auto& sc = slotControls[i];
        char header[64];
        std::snprintf(header, sizeof(header), "Material [%d]", i);

        if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID(i);

            auto* slotAsset = sme->getMaterialAsset(i);
            if (sc.lastKnownAsset != slotAsset)
            {
                sc.lastKnownAsset = slotAsset;
                sc.assetRef->setAssetReference(slotAsset);
            }

            constexpr float LW2 = 120.f;
            if (ImGui::BeginTable("##sl", 2, ImGuiTableFlags_None))
            {
                ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW2);
                ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Material:");
                ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);

                if (sc.assetRef->drawControl(""))
                {
                    auto* newAsset = dynamic_cast<MMaterialAsset*>(
                                         sc.assetRef->getAssetReference());
                    sme->setMaterialAsset(newAsset, i);
                    sc.lastKnownAsset = newAsset;
                }
                ImGui::EndTable();
            }

            // Hint to the user where to edit properties.
            if (slotAsset)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
                ImGui::TextUnformatted("  Select the material asset to edit its properties.");
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }
    }
}