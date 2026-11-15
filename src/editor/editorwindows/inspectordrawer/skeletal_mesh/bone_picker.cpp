#include "bone_picker.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "core/engine/3d/skeletal/skeleton.h"
#include "imgui.h"

namespace
{
    bool containsNoCase(const std::string& haystack, const std::string& needle)
    {
        if (needle.empty()) return true;
        auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                              [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); });
        return it != haystack.end();
    }
}

namespace MBonePicker
{
    bool draw(const char* id, const SSkeleton* skeleton, SString& boneName)
    {
        static char filter[64] = {};
        bool changed = false;

        const bool missing = !boneName.empty() && (!skeleton || skeleton->findBone(boneName) < 0);
        const char* preview = boneName.empty() ? "(none)" : boneName.c_str();

        if (missing)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.35f, 0.35f, 1.0f));
        ImGui::SetNextItemWidth(-FLT_MIN);
        const bool open = ImGui::BeginCombo(id, preview, ImGuiComboFlags_HeightLarge);
        if (missing)
        {
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(skeleton ? "No bone of this name in the skeleton" : "The parent has no skeleton");
        }
        if (!open)
            return false;

        if (ImGui::IsWindowAppearing())
        {
            filter[0] = '\0';
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##bone_filter", "Filter...", filter, sizeof(filter));
        ImGui::Separator();

        if (skeleton)
        {
            bool anyDeform = false;
            for (const auto& bone : skeleton->bones) anyDeform |= bone.isDeformBone;

            // Deform bones first, then the other nodes (armature, helpers).
            for (int pass = 0; pass < 2; ++pass)
            {
                if (pass == 1 && anyDeform)
                {
                    ImGui::Separator();
                    ImGui::TextDisabled("Other nodes");
                }
                for (int i = 0; i < skeleton->getBoneCount(); ++i)
                {
                    const SBone& bone = skeleton->bones[i];
                    const bool firstPass = !anyDeform || bone.isDeformBone;
                    if ((pass == 0) != firstPass) continue;
                    if (pass == 1 && !anyDeform) continue;
                    if (!containsNoCase(bone.name.str(), filter)) continue;

                    ImGui::PushID(i);
                    const bool selected = bone.name == boneName;
                    if (ImGui::Selectable(bone.name.c_str(), selected))
                    {
                        boneName = bone.name;
                        changed = true;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Parent the entity to a Skeletal Mesh first");
        }

        ImGui::EndCombo();
        return changed;
    }
}
