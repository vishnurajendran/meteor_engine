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
    meshAssetControl->canAcceptAssetFuncCallback = [](const TAssetHandle<MAsset>& asset)
    { return dynamic_cast<MStaticMeshAsset*>(asset.get()) != nullptr; };
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
        sc.assetRef->canAcceptAssetFuncCallback = [](const TAssetHandle<MAsset>& asset)
        { return dynamic_cast<MMaterialAsset*>(asset.get()) != nullptr; };
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

            const auto cur        = sme->getStaticMeshAsset();      // TAssetHandle<MStaticMeshAsset>
            const auto controlRef = meshAssetControl->getAssetReference(); // TAssetHandle<MAsset>

            // FIX (A): compare by asset ID — different handle types can't use operator!=
            if (controlRef.getAssetId() != cur.getAssetId())
                // FIX (B): cur IS the handle — pass it directly.
                // Converting constructor: TAssetHandle<MStaticMeshAsset> -> TAssetHandle<MAsset>
                meshAssetControl->setAssetReference(cur);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mesh:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            if (meshAssetControl->drawControl(""))
            {
                // Converting constructor: TAssetHandle<MAsset> → TAssetHandle<MStaticMeshAsset>
                // If the asset isn't actually a MStaticMeshAsset, get() returns nullptr
                // and setStaticMeshAsset handles the null case.
                TAssetHandle<MStaticMeshAsset> meshRef = meshAssetControl->getAssetReference();
                sme->setStaticMeshAsset(meshRef);
            }

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

            // FIX (C): getMaterialAsset returns a handle, not a pointer
            auto slotAsset = sme->getMaterialAsset(i);  // TAssetHandle<MMaterialAsset>

            // FIX (D): lastKnownAsset is now TAssetHandle<MMaterialAsset> — same-type comparison works
            if (sc.lastKnownAsset != slotAsset)
            {
                sc.lastKnownAsset = slotAsset;
                // Converting constructor: TAssetHandle<MMaterialAsset> → TAssetHandle<MAsset>
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
                    // FIX (E): converting constructor replaces dynamic_cast
                    // TAssetHandle<MAsset> → TAssetHandle<MMaterialAsset>
                    TAssetHandle<MMaterialAsset> newAsset = sc.assetRef->getAssetReference();
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