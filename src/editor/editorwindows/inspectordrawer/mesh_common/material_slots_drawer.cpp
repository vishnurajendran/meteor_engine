#include "material_slots_drawer.h"

#include <cstdio>

#include "core/engine/3d/common/mesh_entity_base.h"
#include "core/memory/mnew.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "imgui.h"

void MMaterialSlotsDrawer::ensureSlotControls(int count)
{
    while (static_cast<int>(slotControls.size()) < count)
    {
        SSlotControls sc;
        sc.assetRef = mnew<MAssetReferenceControl>();
        sc.assetRef->canAcceptAssetFuncCallback = [](const TAssetHandle<MAsset>& asset)
        { return dynamic_cast<MMaterialAsset*>(asset.get()) != nullptr; };
        slotControls.push_back(std::move(sc));
    }
}

void MMaterialSlotsDrawer::draw(MMeshEntityBase* entity)
{
    if (!entity)
        return;

    const int slotCount = entity->getMaterialSlotCount();
    ensureSlotControls(slotCount);

    char matHeader[64];
    std::snprintf(matHeader, sizeof(matHeader), "%d Material Slots", slotCount);

    if (!ImGui::CollapsingHeader(matHeader, ImGuiTreeNodeFlags_DefaultOpen))
        return;

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
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MAT_SLOT_SWAP"))
            {
                int srcIndex = *(const int*)payload->Data;
                if (srcIndex != i)
                {
                    entity->swapMaterialSlots(srcIndex, i);
                    slotControls[srcIndex].lastKnownAsset = TAssetHandle<MMaterialAsset>();
                    sc.lastKnownAsset = TAssetHandle<MMaterialAsset>();
                }
            }
            ImGui::EndDragDropTarget();
        }

        // -- Asset reference picker on the same row ---------------------------
        ImGui::SameLine();

        auto slotAsset = entity->getMaterialAsset(i);
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
            entity->setMaterialAsset(newAsset, i);
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
