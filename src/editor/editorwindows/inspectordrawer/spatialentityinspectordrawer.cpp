//
// Created by Vishnu Rajendran on 2024-09-28.
//

#include "spatialentityinspectordrawer.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scripting/lua/lua_script_asset.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/composition/composition_asset.h"
#include "core/engine/composition/composition_utility.h"
#include "editor/app/editorapplication.h"

#include "imgui.h"
#include "imgui-SFML.h"
#include "misc/cpp/imgui_stdlib.h"

#include <typeindex>
#include <cctype>

// ---------------------------------------------------------------------------
// Display-name formatting helpers
// ---------------------------------------------------------------------------

// Converts camelCase or PascalCase into "Pascal Case With Spaces".
// Examples: "dopplerStrength" -> "Doppler Strength"
//           "useSpatial"     -> "Use Spatial"
//           "rollOff"        -> "Roll Off"
//           "loop"           -> "Loop"
static std::string formatDisplayName(const std::string& raw)
{
    if (raw.empty()) return raw;

    std::string result;
    result += static_cast<char>(std::toupper(raw[0]));

    for (size_t i = 1; i < raw.size(); ++i)
    {
        if (std::isupper(raw[i]) && i > 0 && std::islower(raw[i - 1]))
            result += ' ';
        result += raw[i];
    }
    return result;
}

// Strips the leading "M" prefix (when followed by an uppercase letter)
// and inserts spaces between PascalCase words.
// Example: "MAudioSource" -> "Audio Source"
static std::string formatTypeName(const char* raw)
{
    std::string name(raw);
    if (name.size() >= 2 && name[0] == 'M' && std::isupper(name[1]))
        name = name.substr(1);

    std::string result;
    result += name[0];
    for (size_t i = 1; i < name.size(); ++i)
    {
        if (std::isupper(name[i]) && i > 0 && std::islower(name[i - 1]))
            result += ' ';
        result += name[i];
    }
    return result;
}

// ---------------------------------------------------------------------------
// Supported-type dispatch map (built once, never modified)
// ---------------------------------------------------------------------------

const std::unordered_map<std::type_index, MSpatialEntityInspectorDrawer::FieldDrawFn>
MSpatialEntityInspectorDrawer::fieldDrawerMap =
{
    { std::type_index(typeid(Field<float>)), [](MSpatialEntityInspectorDrawer*, FieldBase* f) -> bool {
        auto* tf = static_cast<Field<float>*>(f);
        float val = tf->get();
        if (ImGui::DragFloat("##v", &val, 0.01f, 0.0f, 0.0f, "%.3f")) {
            tf->set(val);
            return true;
        }
        return false;
    }},

    { std::type_index(typeid(Field<int>)), [](MSpatialEntityInspectorDrawer*, FieldBase* f) -> bool {
        auto* tf = static_cast<Field<int>*>(f);
        int val = tf->get();
        if (ImGui::DragInt("##v", &val)) {
            tf->set(val);
            return true;
        }
        return false;
    }},

    { std::type_index(typeid(Field<bool>)), [](MSpatialEntityInspectorDrawer*, FieldBase* f) -> bool {
        auto* tf = static_cast<Field<bool>*>(f);
        bool val = tf->get();
        if (ImGui::Checkbox("##v", &val)) {
            tf->set(val);
            return true;
        }
        return false;
    }},

    { std::type_index(typeid(Field<std::string>)), [](MSpatialEntityInspectorDrawer*, FieldBase* f) -> bool {
        auto* tf = static_cast<Field<std::string>*>(f);
        std::string val = tf->get();
        if (ImGui::InputText("##v", &val)) {
            tf->set(val);
            return true;
        }
        return false;
    }},

    { std::type_index(typeid(Field<SVector3>)), [](MSpatialEntityInspectorDrawer* drawer, FieldBase* f) -> bool {
        auto* tf = static_cast<Field<SVector3>*>(f);
        SVector3 val = tf->get();
        if (drawer->drawXYZComponent(SString(formatDisplayName(f->name)), val)) {
            tf->set(val);
            return true;
        }
        return false;
    }},

    { std::type_index(typeid(Field<SVector2>)), [](MSpatialEntityInspectorDrawer* drawer, FieldBase* f) -> bool {
        auto* tf = static_cast<Field<SVector2>*>(f);
        SVector2 val = tf->get();
        if (drawer->drawXYComponent(SString(formatDisplayName(f->name)), val)) {
            tf->set(val);
            return true;
        }
        return false;
    }},
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MSpatialEntityInspectorDrawer::MSpatialEntityInspectorDrawer() {
    // Set up the persistent script-reference control.
    // The accept callback filters drag-drop to only allow .lua script assets.
    scriptRefControl = new MAssetReferenceControl();
    scriptRefControl->canAcceptAssetFuncCallback = [](TAssetHandle<MAsset> asset) {
        return dynamic_cast<MLuaScriptAsset*>(asset.get()) != nullptr;
    };

    // Resetting a composition instance replaces its entities — move the
    // selection to the new root if it pointed anywhere inside the old one,
    // otherwise it would dangle once the scene frees the old entities.
    // (Set here because this drawer is the editor code that owns Reset; every
    // drawer subclass re-assigns the same lambda, which is harmless.)
    MCompositionUtility::onInstanceReplaced = [](MSpatialEntity* oldRoot, MSpatialEntity* newRoot)
    {
        auto* selected = dynamic_cast<MSpatialEntity*>(MEditorApplication::SelectedObject);
        for (MSpatialEntity* e = selected; e; e = e->getParent())
        {
            if (e == oldRoot)
            {
                MEditorApplication::SelectedObject = newRoot;
                return;
            }
        }
    };
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

    // Reset rebuilds the instance — `target` is stale after that, so stop here;
    // the new root is selected and drawn next frame.
    if (drawCompositionField(target))
        return;

    drawTransformField(target);
    drawScriptField(target);
}

// ---------------------------------------------------------------------------
// Composition (prefab) instance controls
// ---------------------------------------------------------------------------

bool MSpatialEntityInspectorDrawer::drawCompositionField(MSpatialEntity* target)
{
    MSpatialEntity* root = MCompositionUtility::findInstanceRoot(target);
    if (!root)
        return false;

    if (!ImGui::CollapsingHeader("Composition", ImGuiTreeNodeFlags_DefaultOpen))
        return false;

    MCompositionAsset* asset = MCompositionUtility::getAsset(root);

    // Serialises the instance subtree to compare hashes — only for the
    // selected entity, so it's one subtree per frame.
    const ECompositionInstanceState state = MCompositionUtility::getState(root);

    ImGui::TextDisabled("Asset:");
    ImGui::SameLine();
    ImGui::TextUnformatted(asset ? asset->getPath().c_str() : "(missing)");

    if (root != target)
    {
        ImGui::TextDisabled("Instance root:");
        ImGui::SameLine();
        ImGui::TextUnformatted(root->getName().c_str());
    }

    const char* stateLabel = "";
    ImVec4      stateColor = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    switch (state)
    {
    case ECompositionInstanceState::Synced:       stateLabel = "Synced";                  stateColor = ImVec4(0.45f, 0.80f, 0.45f, 1.0f); break;
    case ECompositionInstanceState::Modified:     stateLabel = "Modified (unsaved)";      stateColor = ImVec4(0.95f, 0.80f, 0.30f, 1.0f); break;
    case ECompositionInstanceState::OutOfDate:    stateLabel = "Out of date";             stateColor = ImVec4(0.95f, 0.55f, 0.25f, 1.0f); break;
    case ECompositionInstanceState::MissingAsset: stateLabel = "Asset missing";           stateColor = ImVec4(0.95f, 0.35f, 0.35f, 1.0f); break;
    default: break;
    }
    ImGui::TextDisabled("State:");
    ImGui::SameLine();
    ImGui::TextColored(stateColor, "%s", stateLabel);

    // Saving / resetting during play would bake runtime values into the asset
    // (or rebuild entities mid-simulation), so both are editor-time only.
    const auto* app        = dynamic_cast<MEditorApplication*>(MApplication::getAppInstance());
    const bool  simulating = app && (app->isSimulating() || app->isPaused());
    const bool  hasAsset   = asset && asset->isValid();

    bool replaced = false;
    const float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;

    ImGui::BeginDisabled(simulating || !hasAsset);
    if (ImGui::Button("Save##comp", ImVec2(buttonWidth, 0)))
        MCompositionUtility::saveInstance(root);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Write this instance to the .comp and update every other unmodified instance.");

    ImGui::SameLine();
    if (ImGui::Button("Reset##comp", ImVec2(buttonWidth, 0)))
        replaced = MCompositionUtility::resetInstance(root) != nullptr;
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Discard changes and rebuild this instance from the .comp.\n"
                          "Name, enabled state and root transform are kept.");
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(simulating);
    if (ImGui::Button("Unlink##comp", ImVec2(buttonWidth, 0)))
        MCompositionUtility::unlink(root);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Break the link to the .comp — the entities stay as plain entities.");
    ImGui::EndDisabled();

    return replaced;
}

// ---------------------------------------------------------------------------
// Fallback field drawing (only runs when no custom drawer matched)
// ---------------------------------------------------------------------------

void MSpatialEntityInspectorDrawer::drawDefaultFields(MSpatialEntity* target) {
    drawFields(target);
}

// ---------------------------------------------------------------------------
// Generic field drawing
// ---------------------------------------------------------------------------

void MSpatialEntityInspectorDrawer::drawFields(MSpatialEntity* target)
{
    const auto& fields = target->getFields();
    if (fields.empty())
        return;

    // Strip the "M" prefix and space out PascalCase words for the header.
    const std::string headerName = formatTypeName(target->typeInfo().name);
    if (!ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Label column width -- wide enough for most two-word formatted names.
    // Very long names will clip, which is acceptable.
    const float labelWidth = 110.0f;

    for (auto* f : fields)
    {
        ImGui::PushID(f);

        const std::type_index tid(typeid(*f));
        auto it = fieldDrawerMap.find(tid);

        if (it != fieldDrawerMap.end())
        {
            // Vector types produce their own multi-column table layout,
            // so they are called standalone. Scalar types get wrapped in
            // a shared two-column label | widget table.
            const bool isVector =
                tid == std::type_index(typeid(Field<SVector3>)) ||
                tid == std::type_index(typeid(Field<SVector2>));

            if (isVector)
            {
                it->second(this, f);
            }
            else
            {
                const std::string displayName = formatDisplayName(f->name);

                if (ImGui::BeginTable("##field_row", 2, ImGuiTableFlags_None,
                                      ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    ImGui::TableSetupColumn("lbl", ImGuiTableColumnFlags_WidthFixed,
                                            labelWidth);
                    ImGui::TableSetupColumn("val", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(displayName.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);

                    it->second(this, f);

                    ImGui::EndTable();
                }
            }
        }
        // Unsupported types are silently skipped. Add an else branch here
        // if you want a visible placeholder (e.g. a grayed-out label).

        ImGui::PopID();
    }
}

// ---------------------------------------------------------------------------
// Script reference field
// ---------------------------------------------------------------------------

void MSpatialEntityInspectorDrawer::drawScriptField(MSpatialEntity* target)
{
    if (!ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Read the asset ID directly from the field, no resolution needed.
    // This is stable regardless of which asset manager path resolves.
    SString entityScriptId = target->getScriptAssetId();

    // Read what the control currently references
    auto controlHandle = scriptRefControl->getAssetReference();
    SString controlId  = controlHandle.isNull() ? SString() : controlHandle.getAssetId();

    // Sync the control to match the entity's stored reference.
    // This runs after scene load, undo, or any external change to the field.
    if (entityScriptId != controlId)
    {
        if (!entityScriptId.empty())
        {
            // Resolve the entity's stored asset ID through the asset manager
            // and push the result into the control so it displays correctly.
            auto* am = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
            if (am)
            {
                auto handle = am->getAssetById<MAsset>(entityScriptId);
                if (!handle.isNull())
                    scriptRefControl->setAssetReference(handle);
            }
        }
        else
        {
            delete scriptRefControl;
            scriptRefControl = new MAssetReferenceControl();
            scriptRefControl->canAcceptAssetFuncCallback = [](TAssetHandle<MAsset> asset) {
                return dynamic_cast<MLuaScriptAsset*>(asset.get()) != nullptr;
            };
        }
    }

    if (scriptRefControl->drawCompactControl("Script Asset"))
    {
        MAsset* newAsset = scriptRefControl->getAssetReference().get();
        target->setScriptAssetRef(newAsset);
    }
}

// ---------------------------------------------------------------------------
// Shared light helper
// ---------------------------------------------------------------------------

bool MSpatialEntityInspectorDrawer::drawLightIntensityAndColor(float& intensity, SColor& color) {
    bool changed = false;
    // Two-column table: left col = label, right col = widget.
    // This keeps Intensity and Color labels pixel-aligned regardless of text length.
    constexpr float LABEL_COL_WIDTH = 80.0f;
    if (!ImGui::BeginTable("##LightProps", 2,
                           ImGuiTableFlags_None,
                           ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        return false;

    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed,
                            LABEL_COL_WIDTH);
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
    // Show a compact swatch. Clicking it opens a popup with the full picker --
    // this keeps the panel compact when the user doesn't need to edit color.
    float cols[4] = { color.r, color.g, color.b, color.a };
    const ImVec2 swatchSize = { ImGui::GetContentRegionAvail().x, 20.0f };
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
    ImGui::PushID(&label);
    auto size = ImGui::GetContentRegionAvail();

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::Text("%s", label.c_str());
    ImGui::SameLine();

    std::string name = text;
    ImGui::SetNextItemWidth(size.x - 20.0f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
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
        // This is a direct construction -- no delta, no re-read from the entity --
        // so there is nothing to round-trip through quat -> Euler.
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
    const float badgeW   = ImGui::GetFrameHeight();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 5.0f));

    const ImGuiTableFlags flags = ImGuiTableFlags_None;
    if (!ImGui::BeginTable(STR("##xyz_" + label.str()).c_str(), 4, flags)) {
        ImGui::PopStyleVar();
        return false;
    }

    ImGui::TableSetupColumn("lbl", ImGuiTableColumnFlags_WidthFixed,   72.0f);
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
        ImGui::SameLine(0.0f, 2.0f);
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

bool MSpatialEntityInspectorDrawer::drawXYComponent(const SString& label, SVector2& value) {
    const float badgeW = ImGui::GetFrameHeight();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 5.0f));

    if (!ImGui::BeginTable(STR("##xy_" + label.str()).c_str(), 3, ImGuiTableFlags_None)) {
        ImGui::PopStyleVar();
        return false;
    }

    ImGui::TableSetupColumn("lbl", ImGuiTableColumnFlags_WidthFixed,   72.0f);
    ImGui::TableSetupColumn("x",   ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("y",   ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.c_str());

    auto drawAxis = [&](int col, const char* badge, ImVec4 badgeColor,
                        float& component, const char* id) -> bool {
        ImGui::TableSetColumnIndex(col);
        drawColoredBoxOverLabel(badge, badgeColor, badgeW);
        ImGui::SameLine(0.0f, 2.0f);
        ImGui::SetNextItemWidth(-FLT_MIN);
        return ImGui::DragFloat(id, &component, 0.1f, 0.0f, 0.0f, "%.2f");
    };

    const bool res1 = drawAxis(1, "X", ImVec4(0.75f, 0.12f, 0.12f, 1.0f), value.x,
                               STR("##" + label.str() + "X").c_str());
    const bool res2 = drawAxis(2, "Y", ImVec4(0.12f, 0.65f, 0.12f, 1.0f), value.y,
                               STR("##" + label.str() + "Y").c_str());

    ImGui::EndTable();
    ImGui::PopStyleVar();
    return res1 || res2;
}