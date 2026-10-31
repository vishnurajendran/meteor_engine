//
// Created by Vishnu Rajendran on 2024-09-28.
//

#include "spatialentityinspectordrawer.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scripting/lua/lua_script_asset.h"
#include "core/engine/scripting/lua/lua_script_registry.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/composition/composition_asset.h"
#include "core/engine/composition/composition_utility.h"
#include "editor/app/editorapplication.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"

#include "imgui.h"
#include "imgui-SFML.h"
#include "misc/cpp/imgui_stdlib.h"

#include <typeindex>
#include <cctype>
#include <algorithm>
#include "core/memory/mnew.h"

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
        if (MInspectorField::DragFloat("##v", &val, 0.01f, 0.0f, 0.0f, "%.3f")) {
            tf->set(val);
            return true;
        }
        return false;
    }},

    { std::type_index(typeid(Field<int>)), [](MSpatialEntityInspectorDrawer*, FieldBase* f) -> bool {
        auto* tf = static_cast<Field<int>*>(f);
        int val = tf->get();
        if (MInspectorField::DragInt("##v", &val)) {
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
    scriptRefControl = mnew<MAssetReferenceControl>();
    scriptRefControl->canAcceptAssetFuncCallback = [](TAssetHandle<MAsset> asset) {
        return dynamic_cast<MLuaScriptAsset*>(asset.get()) != nullptr;
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
        START_FIELD_DRAW("Enabled")
            bool enabled = target->getEnabled();
            if (ImGui::Checkbox("##v", &enabled))
                target->setEnabled(enabled);
        END_FIELD_DRAW()

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

    START_FIELD_DRAW("Asset")
        if (asset)
            ImGui::TextUnformatted(asset->getPath().c_str());
        else
            MInspectorField::statusText(EInspectorStatus::Error, "(missing)",
                                        "The .comp this instance came from can't be found.");
    END_FIELD_DRAW()

    if (root != target)
    {
        START_FIELD_DRAW("Instance Root")
            ImGui::TextUnformatted(root->getName().c_str());
        END_FIELD_DRAW()
    }

    const char*      stateLabel   = "";
    const char*      stateTooltip = nullptr;
    EInspectorStatus stateStatus  = EInspectorStatus::Info;
    switch (state)
    {
    case ECompositionInstanceState::Synced:
        stateLabel   = "Synced";
        stateStatus  = EInspectorStatus::Info;
        stateTooltip = "This instance matches its .comp.";
        break;
    case ECompositionInstanceState::Modified:
        stateLabel   = "Modified (unsaved)";
        stateStatus  = EInspectorStatus::Warning;
        stateTooltip = "This instance has changes that aren't saved to the .comp.\n"
                       "Save to write them, or Reset to discard them.";
        break;
    case ECompositionInstanceState::OutOfDate:
        stateLabel   = "Out of date";
        stateStatus  = EInspectorStatus::Warning;
        stateTooltip = "The .comp changed since this instance was built.\n"
                       "Reset to rebuild it from the .comp.";
        break;
    case ECompositionInstanceState::MissingAsset:
        stateLabel   = "Asset missing";
        stateStatus  = EInspectorStatus::Error;
        stateTooltip = "The .comp this instance came from can't be found.\n"
                       "Unlink to keep the entities as plain entities.";
        break;
    default: break;
    }
    START_FIELD_DRAW("State")
        MInspectorField::statusText(stateStatus, stateLabel, stateTooltip);
    END_FIELD_DRAW()

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

    // Skip the section entirely when none of the fields have a drawer (e.g.
    // only internal/unsupported types), so it doesn't show an empty header.
    const bool anyDrawable = std::any_of(fields.begin(), fields.end(), [](FieldBase* f) {
        return fieldDrawerMap.count(std::type_index(typeid(*f))) > 0;
    });
    if (!anyDrawable)
        return;

    // Strip the "M" prefix and space out PascalCase words for the header.
    const std::string headerName = formatTypeName(target->typeInfo().name);
    if (!ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        return;

    for (auto* f : fields)
    {
        ImGui::PushID(f);

        const std::type_index tid(typeid(*f));
        auto it = fieldDrawerMap.find(tid);

        if (it != fieldDrawerMap.end())
        {
            // Vector drawers lay out their own field row (label + axes).
            const bool isVector =
                tid == std::type_index(typeid(Field<SVector3>)) ||
                tid == std::type_index(typeid(Field<SVector2>));

            if (isVector)
            {
                it->second(this, f);
            }
            else
            {
                START_FIELD_DRAW(formatDisplayName(f->name))
                    it->second(this, f);
                END_FIELD_DRAW()
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

    // Keep the control on the entity's stored asset ID (after scene load,
    // undo, or any external change). The raw ID is used rather than a resolved
    // asset so a missing script still shows as an error in the field.
    const SString entityScriptId = target->getScriptAssetId();
    if (scriptRefControl->getAssetId() != entityScriptId)
        scriptRefControl->setAssetId(entityScriptId);

    START_FIELD_DRAW("Script Asset")
        if (scriptRefControl->drawCompactControl(""))
        {
            MAsset* newAsset = scriptRefControl->getAssetReference().get();
            target->setScriptAssetRef(newAsset);
        }
    END_FIELD_DRAW()

    drawBehaviourTuning(target);
}

// ---------------------------------------------------------------------------
// Behaviour tuning - the script's public fields, editable per entity
// ---------------------------------------------------------------------------

void MSpatialEntityInspectorDrawer::drawBehaviourTuning(MSpatialEntity* target)
{
    auto* script = dynamic_cast<MLuaScriptAsset*>(target->getScriptAssetRef());
    if (!script)
        return;

    // Published by the editor domain's script refresh.
    const SLuaScriptFieldSet* fieldSet = MLuaScriptRegistry::getPublicFields(script);
    if (!fieldSet)
    {
        ImGui::TextDisabled("Compiling script...");
        return;
    }
    if (!fieldSet->compiled)
    {
        ImGui::TextDisabled("Script failed to compile - see the console");
        return;
    }
    if (fieldSet->fields.empty())
        return;

    if (!ImGui::CollapsingHeader("Behaviour Tuning", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    const SString scriptId = target->getScriptAssetId();
    const ImVec4  overriddenColor(0, 1, 1, 1.0f);
    const ImVec4  normalColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    const float   resetWidth  = MInspectorField::getResetButtonWidth();

    for (const auto& field : fieldSet->fields)
    {
        ImGui::PushID(field.name.c_str());

        const EDynValueType type = field.defaultValue.type;

        // An override whose type no longer matches the script (the script
        // changed) is ignored here; editing replaces it.
        const SDynValue* stored = target->getTuningOverride(scriptId, field.name);
        const bool overridden = stored && (stored->type == type ||
            ((stored->type == EDynValueType::Integer || stored->type == EDynValueType::Float) &&
             (type == EDynValueType::Integer || type == EDynValueType::Float)));

        SDynValue value = field.defaultValue;
        if (overridden)
        {
            // Show a number override in the field's current form (int / float).
            if (type == EDynValueType::Integer && stored->type == EDynValueType::Float)
                value.setInt(static_cast<int>(std::get<float>(stored->value)));
            else if (type == EDynValueType::Float && stored->type == EDynValueType::Integer)
                value.setFloat(static_cast<float>(std::get<int>(stored->value)));
            else
                value = *stored;
        }

        // Label is highlighted when this entity overrides the script default.
        const std::string label = formatDisplayName(field.name.str());
        // Every row reserves the reset cell so the columns stay even.
        MInspectorField::setNextFieldTrailingCell(resetWidth);
        START_FIELD_DRAW(label, overridden ? overriddenColor : normalColor,
                         overridden ? "Overridden on this entity" : nullptr)

            bool changed = false;
            switch (type)
            {
                case EDynValueType::Bool:
                {
                    bool v = std::get<bool>(value.value);
                    if ((changed = ImGui::Checkbox("##v", &v)))
                        value.setBool(v);
                    break;
                }
                case EDynValueType::Integer:
                {
                    int v = std::get<int>(value.value);
                    if ((changed = MInspectorField::DragInt("##v", &v, 0.1f)))
                        value.setInt(v);
                    break;
                }
                case EDynValueType::Float:
                {
                    float v = std::get<float>(value.value);
                    if ((changed = MInspectorField::DragFloat("##v", &v, 0.01f, 0.0f, 0.0f, "%.3f")))
                        value.setFloat(v);
                    break;
                }
                case EDynValueType::String:
                {
                    std::string v = std::get<std::string>(value.value);
                    if ((changed = ImGui::InputText("##v", &v)))
                        value.setString(v);
                    break;
                }
                default:
                    break;
            }
            if (changed)
                target->setTuningOverride(scriptId, field.name, value);

            if (overridden)
            {
                MInspectorField::trailingCell();
                if (MInspectorField::ResetButton("##reset", "Back to the script's default"))
                    target->clearTuningOverride(scriptId, field.name);
            }
        END_FIELD_DRAW()
        ImGui::PopID();
    }
}

// ---------------------------------------------------------------------------
// Shared light helper
// ---------------------------------------------------------------------------

bool MSpatialEntityInspectorDrawer::drawLightIntensityAndColor(float& intensity, SColor& color) {
    bool changed = false;

    START_FIELD_DRAW("Intensity")
        if (MInspectorField::DragFloat("##v", &intensity, 0.01f, 0.0f, FLT_MAX, "%.2f"))
            changed = true;
    END_FIELD_DRAW()

    START_FIELD_DRAW("Color")
        // Show a compact swatch. Clicking it opens a popup with the full picker --
        // this keeps the panel compact when the user doesn't need to edit color.
        float cols[4] = { color.r, color.g, color.b, color.a };
        const ImVec2 swatchSize = { ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() };
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
    END_FIELD_DRAW()

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
                      0.0f);

    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos  = ImVec2(pos.x + (boxWidth  - textSize.x) * 0.5f,
                             pos.y + (frameH - textSize.y) * 0.5f);
    dl->AddText(textPos, IM_COL32_WHITE, label);

    ImGui::Dummy(ImVec2(boxWidth, frameH));
}

bool MSpatialEntityInspectorDrawer::drawTextField(const SString& label, SString& text) {
    bool res = false;
    START_FIELD_DRAW(label.c_str())
        std::string value = text;
        res = ImGui::InputText("##v", &value, ImGuiInputTextFlags_EnterReturnsTrue);
        if (res)
            text = value;
    END_FIELD_DRAW()
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

bool MSpatialEntityInspectorDrawer::drawAxisRow(const SString& label, float* components, int count) {
    static constexpr const char* kBadges[] = { "X", "Y", "Z" };
    static const ImVec4 kColors[] = {
        ImVec4(0.75f, 0.12f, 0.12f, 1.0f),
        ImVec4(0.12f, 0.65f, 0.12f, 1.0f),
        ImVec4(0.12f, 0.25f, 0.75f, 1.0f),
    };

    bool changed = false;
    START_FIELD_DRAW(label.c_str())
        // Split the value column evenly between the axes.
        const float gap    = ImGui::GetStyle().ItemInnerSpacing.x;
        const float badgeW = ImGui::GetFrameHeight();
        const float cellW  = (ImGui::GetContentRegionAvail().x - gap * (count - 1)) / count;

        for (int i = 0; i < count; ++i)
        {
            if (i > 0)
                ImGui::SameLine(0.0f, gap);
            ImGui::PushID(i);
            drawColoredBoxOverLabel(kBadges[i], kColors[i], badgeW);
            ImGui::SameLine(0.0f, 2.0f);
            ImGui::SetNextItemWidth(std::max(cellW - badgeW - 2.0f, 1.0f));
            if (MInspectorField::DragFloat("##v", &components[i], 0.1f, 0.0f, 0.0f, "%.2f"))
                changed = true;
            ImGui::PopID();
        }
    END_FIELD_DRAW()
    return changed;
}

bool MSpatialEntityInspectorDrawer::drawXYZComponent(const SString& label, SVector3& value) {
    return drawAxisRow(label, &value.x, 3);
}

bool MSpatialEntityInspectorDrawer::drawXYComponent(const SString& label, SVector2& value) {
    return drawAxisRow(label, &value.x, 2);
}
