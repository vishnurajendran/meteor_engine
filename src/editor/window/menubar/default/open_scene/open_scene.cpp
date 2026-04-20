#include "open_scene.h"

#include "core/engine/scene/scene_io.h"
#include "editor/window/menubar/menubartree.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

static constexpr const char* POPUP_ID = "Open Scene##popup";
static std::string s_pathBuf;

bool MOpenSceneMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MOpenSceneMenubarItem());
    return true;
}();

int MOpenSceneMenubarItem::getPriority() const
{
    return MMenubarItem::PRIORITY_HIGHEST + 1;
}

SString MOpenSceneMenubarItem::getPath() const
{
    return "File/Open Scene";
}

void MOpenSceneMenubarItem::onSelect()
{
    // Pre-fill with the current path if available
    s_pathBuf = MSceneIO::getCurrentScenePath().str();
    ImGui::OpenPopup(POPUP_ID);
}

void MOpenSceneMenubarItem::drawPopup()
{
    // Centre the popup on first use
    ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520, 0), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal(POPUP_ID, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::TextUnformatted("Scene path:");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##scenepath", &s_pathBuf);

    ImGui::Spacing();

    if (ImGui::Button("Open", ImVec2(120, 0)))
    {
        if (!s_pathBuf.empty())
            MSceneIO::loadScene(s_pathBuf.c_str());
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120, 0)))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}