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

    if (ImGui::CollapsingHeader("Static Mesh", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 120.f;
        if (ImGui::BeginTable("##sm", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            const auto cur        = sme->getStaticMeshAsset();
            const auto controlRef = meshAssetControl->getAssetReference();

            if (controlRef.getAssetId() != cur.getAssetId())
                meshAssetControl->setAssetReference(cur);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mesh:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            if (meshAssetControl->drawControl(""))
            {
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

    const int slotCount = sme->getMaterialSlotCount();
    ensureSlotControls(slotCount);

    char matHeader[64];
    std::snprintf(matHeader, sizeof(matHeader), "%d Material Slots", slotCount);

    if (ImGui::CollapsingHeader(matHeader, ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Compact style for tighter rows.
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 2.f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(4.f, 2.f));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,  ImVec2(4.f, 2.f));

        for (int i = 0; i < slotCount; ++i)
        {
            auto& sc = slotControls[i];
            ImGui::PushID(i);

            {
                ImVec2 rowMin = ImGui::GetCursorScreenPos();
                // Peek ahead to estimate row height (asset control is ~36px).
                float  rowH   = 40.f;
                float  rowW   = ImGui::GetContentRegionAvail().x;
                ImVec2 rowMax = { rowMin.x + rowW, rowMin.y + rowH };

                ImU32 rowBg = (i % 2 == 0)
                    ? IM_COL32(35, 35, 35, 255)
                    : IM_COL32(42, 42, 42, 255);
                ImGui::GetWindowDrawList()->AddRectFilled(rowMin, rowMax, rowBg);
            }

            char label[16];
            std::snprintf(label, sizeof(label), "[%d]", i);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::SetDragDropPayload("MAT_SLOT_SWAP", &i, sizeof(int));
                ImGui::Text("Slot [%d]", i);
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload =
                        ImGui::AcceptDragDropPayload("MAT_SLOT_SWAP"))
                {
                    int srcIndex = *(const int*)payload->Data;
                    if (srcIndex != i)
                    {
                        sme->swapMaterialSlots(srcIndex, i);
                        slotControls[srcIndex].lastKnownAsset = TAssetHandle<MMaterialAsset>();
                        sc.lastKnownAsset = TAssetHandle<MMaterialAsset>();
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // ── Asset reference picker on the same row ───────────────────
            ImGui::SameLine();

            auto slotAsset = sme->getMaterialAsset(i);
            if (sc.lastKnownAsset != slotAsset)
            {
                sc.lastKnownAsset = slotAsset;
                sc.assetRef->setAssetReference(slotAsset);
            }

            // The compact control uses GetContentRegionAvail() for width
            // so it fills the remaining space automatically.
            if (sc.assetRef->drawControl(""))
            {
                TAssetHandle<MMaterialAsset> newAsset = sc.assetRef->getAssetReference();
                sme->setMaterialAsset(newAsset, i);
                sc.lastKnownAsset = newAsset;
            }

            // Thin separator between slots.
            if (i < slotCount - 1)
            {
                ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.2f, 0.2f, 0.2f, 1.f));
                ImGui::Separator();
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }

        ImGui::PopStyleVar(3);
    }
}