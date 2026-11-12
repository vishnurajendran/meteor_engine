#include "post_process_stack_inspector_drawer.h"

#include <cctype>

#include "core/engine/postprocess/post_process_stack_entity.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_registry.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_stack.h"
#include "core/memory/mnew.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/utils/field_engine_types.h"

const bool MPostProcessStackInspectorDrawer::registered = []() {
    registerDrawer(mnew<MPostProcessStackInspectorDrawer>());
    return true;
}();

bool MPostProcessStackInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MPostProcessStackEntity*>(entity) != nullptr;
}

// "smoothness" -> "Smoothness", "bloomThreshold" -> "Bloom Threshold"
static std::string prettyFieldName(const std::string& name)
{
    std::string out;
    for (size_t i = 0; i < name.size(); ++i)
    {
        const char c = name[i];
        if (i == 0)                                   out += static_cast<char>(std::toupper(c));
        else if (std::isupper(static_cast<unsigned char>(c))) { out += ' '; out += c; }
        else                                          out += c;
    }
    return out;
}

void MPostProcessStackInspectorDrawer::drawSettingsFields(SPostProcessEffectSettings* settings)
{
    for (FieldBase* field : settings->getFields())
    {
        if (field->name == "enabled") continue;   // drawn in the effect header

        const auto        meta  = settings->getFieldMeta(field->name);
        const std::string label = prettyFieldName(field->name);
        const char*       tip   = meta.tooltip;

        if (auto* f = dynamic_cast<Field<float>*>(field))
        {
            START_FIELD_DRAW(label, tip)
                float v = f->get();
                const bool changed = (meta.max > meta.min)
                    ? MInspectorField::SliderFloat("##v", &v, meta.min, meta.max, "%.2f")
                    : MInspectorField::DragFloat("##v", &v, meta.speed);
                if (changed) f->set(v);
            END_FIELD_DRAW()
        }
        else if (auto* f = dynamic_cast<Field<int>*>(field))
        {
            START_FIELD_DRAW(label, tip)
                int v = f->get();
                bool changed = false;
                if (!meta.options.empty())
                    changed = ImGui::Combo("##v", &v, meta.options.data(), static_cast<int>(meta.options.size()));
                else
                    changed = MInspectorField::DragInt("##v", &v, 1.0f,
                                                       static_cast<int>(meta.min), static_cast<int>(meta.max));
                if (changed) f->set(v);
            END_FIELD_DRAW()
        }
        else if (auto* f = dynamic_cast<Field<bool>*>(field))
        {
            START_FIELD_DRAW(label, tip)
                bool v = f->get();
                if (ImGui::Checkbox("##v", &v)) f->set(v);
            END_FIELD_DRAW()
        }
        else if (auto* f = dynamic_cast<Field<SVector3>*>(field))
        {
            START_FIELD_DRAW(label, tip)
                SVector3 v = f->get();
                const bool changed = meta.color
                    ? ImGui::ColorEdit3("##v", &v.x, ImGuiColorEditFlags_Float)
                    : ImGui::DragFloat3("##v", &v.x, meta.speed);
                if (changed) f->set(v);
            END_FIELD_DRAW()
        }
    }
}

void MPostProcessStackInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    if (!ImGui::CollapsingHeader("Post Processing", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    auto* stack = MPostProcessStackEntity::getStack();
    if (!stack)
    {
        ImGui::TextDisabled("No post-process stack on the main pipeline.");
        return;
    }
    if (stack->getOwner() != target)
    {
        ImGui::TextDisabled("Another Post Process Stack in the scene is active.");
        return;
    }

    // ---- Effects, in run order ---------------------------------------------
    SString toRemove;
    for (const SString& type : stack->getEffectTypeNames())
    {
        auto* settings = stack->getSettings(type);
        if (!settings) continue;

        ImGui::PushID(type.c_str());

        bool enabled = settings->enabled.get();
        if (ImGui::Checkbox("##enabled", &enabled)) settings->enabled.set(enabled);
        ImGui::SameLine();

        const bool open = ImGui::TreeNodeEx(type.c_str(),
                                            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
        if (ImGui::BeginPopupContextItem("##effectMenu"))
        {
            if (ImGui::MenuItem("Remove")) toRemove = type;
            ImGui::EndPopup();
        }

        if (open)
        {
            if (!enabled) ImGui::BeginDisabled();
            drawSettingsFields(settings);
            if (!enabled) ImGui::EndDisabled();

            if (ImGui::SmallButton("Remove")) toRemove = type;
            ImGui::TreePop();
        }

        ImGui::PopID();
        ImGui::Spacing();
    }

    // Removing invalidates that effect's settings - only after the loop.
    if (!toRemove.empty())
        stack->removeEffect(toRemove);

    // ---- Add Effect -------------------------------------------------------------
    ImGui::Separator();
    if (ImGui::Button("Add Effect", ImVec2(-1, 0)))
        ImGui::OpenPopup("##addPostEffect");

    if (ImGui::BeginPopup("##addPostEffect"))
    {
        bool any = false;
        for (const auto& entry : MPostProcessEffectRegistry::getAll())
        {
            if (stack->hasEffect(entry.name)) continue;
            any = true;
            if (ImGui::Selectable(entry.name.c_str()))
                stack->addEffect(entry.name);
        }
        if (!any) ImGui::TextDisabled("All effects added");
        ImGui::EndPopup();
    }
}
