#include "open_scene.h"

#include <filesystem>
#include "ImGuiFileDialog.h"
#include "editor/window/menubar/menubartree.h"
#include "imgui.h"

#include "editor/helper/scene_io.h"

static constexpr const char* DIALOG_KEY = "OpenSceneDlg";

// Set by onSelect(), consumed by drawPopup() on the same frame.
// This defers OpenDialog() out of the BeginMainMenuBar() context so that
// OpenPopup and BeginPopupModal share the same ImGui window context.
static bool s_wantOpen = false;

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
    // Do NOT call OpenDialog here — this runs inside BeginMainMenuBar(),
    // which would attach OpenPopup to the wrong window context.
    // Just set the flag; drawPopup() will open the dialog next frame.
    s_wantOpen = true;
}

void MOpenSceneMenubarItem::drawPopup()
{
    // Open the dialog from outside any Begin/End so the popup context matches.
    if (s_wantOpen)
    {
        s_wantOpen = false;

        IGFD::FileDialogConfig config;
        config.path              = ".";
        config.countSelectionMax = 1;
        config.flags             = ImGuiFileDialogFlags_Modal;

        ImGuiFileDialog::Instance()->OpenDialog(
            DIALOG_KEY,
            "Open Scene",
            ".scml",
            config
        );
    }

    // Fix: override the deep dark theme's red ModalWindowDimBg with dark grey.
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.05f, 0.05f, 0.05f, 0.7f));

    ImGuiIO& io    = ImGui::GetIO();
    ImVec2 maxSize = io.DisplaySize;
    ImVec2 minSize = { maxSize.x * 0.5f, maxSize.y * 0.5f };

    if (ImGuiFileDialog::Instance()->Display(DIALOG_KEY,
                                              ImGuiWindowFlags_NoCollapse,
                                              minSize, maxSize))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string path = ImGuiFileDialog::Instance()->GetFilePathName();

            // Normalize Windows backslashes → forward slashes
            for (char& c : path)
                if (c == '\\') c = '/';

            // Strip the current working directory prefix so the engine always
            // works with project-relative paths (e.g. "meteor_assets/...").
            // std::filesystem::current_path() already uses forward slashes on
            // all platforms after the normalization above.
            std::string cwd = std::filesystem::current_path().string();
            for (char& c : cwd)
                if (c == '\\') c = '/';

            // Ensure cwd ends with '/' before comparing
            if (!cwd.empty() && cwd.back() != '/')
                cwd += '/';

            if (path.starts_with(cwd))
                path = path.substr(cwd.size());

            if (!path.empty())
                MSceneIO::loadScene(path.c_str());
        }
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::PopStyleColor();
}