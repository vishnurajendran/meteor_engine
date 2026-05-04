//
// Created by Vishnu Rajendran on 2024-09-28.
//

#include "spatialentityinspectordrawer.h"
#include "core/engine/entities/spatial/spatial.h"

#include "imgui.h"
#include "imgui-SFML.h"
#include "misc/cpp/imgui_stdlib.h"

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MSpatialEntityInspectorDrawer::MSpatialEntityInspectorDrawer() {
    // trfTexture removed: it was loaded but never referenced in any draw call.
    // Re-add here if an icon is needed beside the Transform header.
}

// ---------------------------------------------------------------------------
// canDraw / onDrawInspector
// ---------------------------------------------------------------------------

bool MSpatialEntityInspectorDrawer::canDraw(MSpatialEntity* /*entity*/) {
    return true; // fallback drawer accepts everything
}

void MSpatialEntityInspectorDrawer::onDrawInspector(MSpatialEntity* target) {
    if (!target)
        return;

    if (ImGui::CollapsingHeader("Basic Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool enabled = target->getEnabled();
        if (ImGui::Checkbox("Enabled", &enabled))
            target->setEnabled(enabled);

        auto name = target->getName();
        if (drawTextField("Entity Name", name))
            target->setName(name);
    }

    drawTransformField(target);
}

// ---------------------------------------------------------------------------
// Shared light helper
// ---------------------------------------------------------------------------

bool MSpatialEntityInspectorDrawer::drawLightIntensityAndColor(float& intensity, SColor& color) {
    bool changed = false;
    const float dpi = DPIHelper::GetDPIScaleFactor();

    // Two-column table: left col = label, right col = widget.
    // This keeps Intensity and Color labels pixel-aligned regardless of text length.
    constexpr float LABEL_COL_WIDTH = 80.0f;
    if (!ImGui::BeginTable("##LightProps", 2,
                           ImGuiTableFlags_None,
                           ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        return false;

    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed,
                            LABEL_COL_WIDTH * dpi);
    ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

    // --- Intensity row ---
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Intensity");

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##Intensity", &intensity, 0.01f, 0.0f, FLT_MAX, "%.2f"))
        changed = true;

    // --- Color row ---
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Color");

    ImGui::TableSetColumnIndex(1);
    // Show a compact swatch. Clicking it opens a popup with the full picker —
    // this keeps the panel compact when the user doesn't need to edit color.
    float cols[4] = { color.r, color.g, color.b, color.a };
    const ImVec2 swatchSize = { ImGui::GetContentRegionAvail().x, 20.0f * dpi };
    if (ImGui::ColorButton("##ColorSwatch",
                           ImVec4(cols[0], cols[1], cols[2], cols[3]),
                           ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaPreview,
                           swatchSize))
    {
        ImGui::OpenPopup("##ColorPickerPopup");
    }

    if (ImGui::BeginPopup("##ColorPickerPopup")) {
        ImGui::TextDisabled("Light Color");
        ImGui::Separator();
        if (ImGui::ColorPicker4("##LightColorPicker", cols,
                                ImGuiColorEditFlags_Float |
                                ImGuiColorEditFlags_DisplayRGB |
                                ImGuiColorEditFlags_DisplayHex)) {
            color   = SColor(cols[0], cols[1], cols[2], cols[3]);
            changed = true;
        }
        ImGui::EndPopup();
    }

    ImGui::EndTable();
    return changed;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void MSpatialEntityInspectorDrawer::drawColoredBoxOverLabel(const char* label,
                                                            ImVec4 boxColor,
                                                            float boxWidth) {
    const float  frameH  = ImGui::GetFrameHeight();
    ImDrawList*  dl      = ImGui::GetWindowDrawList();
    ImVec2       pos     = ImGui::GetCursorScreenPos();
    ImU32        col32   = ImGui::ColorConvertFloat4ToU32(boxColor);

    dl->AddRectFilled(pos,
                      ImVec2(pos.x + boxWidth, pos.y + frameH),
                      col32,
                      3.0f);

    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos  = ImVec2(pos.x + (boxWidth  - textSize.x) * 0.5f,
                             pos.y + (frameH - textSize.y) * 0.5f);
    dl->AddText(textPos, IM_COL32_WHITE, label);

    ImGui::Dummy(ImVec2(boxWidth, frameH));
}

bool MSpatialEntityInspectorDrawer::drawTextField(const SString& label, SString& text) {
    const float dpi = DPIHelper::GetDPIScaleFactor();
    ImGui::PushID(&label);
    auto size = ImGui::GetContentRegionAvail();

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f * dpi);
    ImGui::Text("%s", label.c_str());
    ImGui::SameLine();

    std::string name = text;
    ImGui::SetNextItemWidth(size.x - 20.0f * dpi);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f * dpi);
    const bool res = ImGui::InputText("##EntityNameField", &name,
                                      ImGuiInputTextFlags_EnterReturnsTrue);
    if (res)
        text = name;

    ImGui::PopID();
    return res;
}

void MSpatialEntityInspectorDrawer::drawTransformField(MSpatialEntity* target)
{
    if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    auto pos     = target->getRelativePosition();
    auto rotQuat = target->getRelativeRotation();
    auto scale   = target->getRelativeScale();

    // --- Euler cache lookup ---------------------------------------------------
    EulerCache& cache = eulerCache_[target];

    // q and -q represent the same rotation; normalise sign before comparing.
    auto canonicalise = [](SQuaternion q) -> SQuaternion {
        return q.w < 0.0f ? -q : q;
    };

    auto quatsEqual = [&](const SQuaternion& a, const SQuaternion& b) -> bool {
        constexpr float eps = 1e-6f;
        SQuaternion ca = canonicalise(a);
        SQuaternion cb = canonicalise(b);
        return std::abs(ca.x - cb.x) < eps
            && std::abs(ca.y - cb.y) < eps
            && std::abs(ca.z - cb.z) < eps
            && std::abs(ca.w - cb.w) < eps;
    };

    // Only re-derive Euler when the entity was moved by something other than
    // this inspector (external change).
    if (!quatsEqual(cache.lastQuat, rotQuat))
    {
        cache.euler   = quaternionToEuler(rotQuat);
        cache.lastQuat = rotQuat;
    }

    // --- Draw fields ----------------------------------------------------------

    if (drawXYZComponent("Position", pos))
        target->setRelativePosition(pos);

    if (drawXYZComponent("Rotation", cache.euler))
    {
        // Rebuild quaternion from the absolute cached Euler angles (YXZ order).
        // This is a direct construction — no delta, no re-read from the entity —
        // so there is nothing to round-trip through quat → Euler.
        const SQuaternion qY = glm::angleAxis(glm::radians(cache.euler.y), SVector3(0, 1, 0));
        const SQuaternion qX = glm::angleAxis(glm::radians(cache.euler.x), SVector3(1, 0, 0));
        const SQuaternion qZ = glm::angleAxis(glm::radians(cache.euler.z), SVector3(0, 0, 1));
        const SQuaternion newQuat = qY * qX * qZ;

        // Keep the cache consistent so next frame we don't misidentify this as
        // an external change and overwrite the Euler values we just edited.
        cache.lastQuat = newQuat;
        target->setRelativeRotation(newQuat);
    }

    if (drawXYZComponent("Scale", scale))
        target->setRelativeScale(scale);
}

bool MSpatialEntityInspectorDrawer::drawXYZComponent(const SString& label, SVector3& value) {
    const float dpi      = DPIHelper::GetDPIScaleFactor();
    const float badgeW   = ImGui::GetFrameHeight();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f * dpi, 5.0f * dpi));

    const ImGuiTableFlags flags = ImGuiTableFlags_None;
    if (!ImGui::BeginTable(STR("##xyz_" + label.str()).c_str(), 4, flags)) {
        ImGui::PopStyleVar();
        return false;
    }

    ImGui::TableSetupColumn("lbl", ImGuiTableColumnFlags_WidthFixed,   72.0f * dpi);
    ImGui::TableSetupColumn("x",   ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("y",   ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("z",   ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.c_str());

    auto drawAxis = [&](int col, const char* badge, ImVec4 badgeColor,
                        float& component, const char* id) -> bool {
        ImGui::TableSetColumnIndex(col);
        drawColoredBoxOverLabel(badge, badgeColor, badgeW);
        ImGui::SameLine(0.0f, 2.0f * dpi);
        ImGui::SetNextItemWidth(-FLT_MIN);
        return ImGui::DragFloat(id, &component, 0.1f, 0.0f, 0.0f, "%.2f");
    };

    const bool res1 = drawAxis(1, "X", ImVec4(0.75f, 0.12f, 0.12f, 1.0f), value.x,
                               STR("##" + label.str() + "X").c_str());
    const bool res2 = drawAxis(2, "Y", ImVec4(0.12f, 0.65f, 0.12f, 1.0f), value.y,
                               STR("##" + label.str() + "Y").c_str());
    const bool res3 = drawAxis(3, "Z", ImVec4(0.12f, 0.25f, 0.75f, 1.0f), value.z,
                               STR("##" + label.str() + "Z").c_str());

    ImGui::EndTable();
    ImGui::PopStyleVar();
    return res1 || res2 || res3;
}