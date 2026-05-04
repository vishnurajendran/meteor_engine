//
// save_scene.cpp
//

#include "save_scene.h"
#include <filesystem>
#include "ImGuiFileDialog.h"
#include "core/engine/scene/scenemanager.h"
#include "editor/editorscenemanager/editorscenemanager.h"
#include "editor/window/menubar/menubartree.h"
#include "imgui.h"

#include "editor/helper/scene_io.h"

// ── Shared helpers ────────────────────────────────────────────────────────────

// Returns the path the active scene was loaded from / last saved to.
// Checks the editor scene manager directly so paths set by the asset window
// (which bypasses MSceneIO) are also visible.
static SString resolveCurrentScenePath()
{
    auto* mgr = dynamic_cast<MEditorSceneManager*>(
        MSceneManager::getSceneManagerInstance());
    if (mgr && !mgr->getActiveScenePath().empty())
        return mgr->getActiveScenePath();
    return MSceneIO::getCurrentPath();   // fallback: MSceneIO-set path
}

static std::string toRelativePath(std::string path)
{
    for (char& c : path) if (c == '\\') c = '/';
    std::string cwd = std::filesystem::current_path().string();
    for (char& c : cwd) if (c == '\\') c = '/';
    if (!cwd.empty() && cwd.back() != '/') cwd += '/';
    if (path.starts_with(cwd)) path = path.substr(cwd.size());
    return path;
}

static void openSaveDialog(const char* dialogKey, const SString& suggestedName = "untitled.scml")
{
    IGFD::FileDialogConfig config;
    config.path              = ".";
    config.fileName          = suggestedName.str();
    config.countSelectionMax = 1;
    config.flags             = ImGuiFileDialogFlags_Modal
                             | ImGuiFileDialogFlags_ConfirmOverwrite;

    ImGuiFileDialog::Instance()->OpenDialog(dialogKey, "Save Scene As", ".scml", config);
}

static bool drawSaveDialog(const char* dialogKey)
{
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.05f, 0.05f, 0.05f, 0.7f));

    ImGuiIO& io    = ImGui::GetIO();
    ImVec2 maxSize = io.DisplaySize;
    ImVec2 minSize = { maxSize.x * 0.5f, maxSize.y * 0.5f };
    bool saved     = false;

    if (ImGuiFileDialog::Instance()->Display(dialogKey,
                                              ImGuiWindowFlags_NoCollapse,
                                              minSize, maxSize))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string path = toRelativePath(
                ImGuiFileDialog::Instance()->GetFilePathName());

            if (!path.empty())
            {
                auto* mgr = dynamic_cast<MEditorSceneManager*>(
                    MSceneManager::getSceneManagerInstance());
                if (mgr)
                {
                    mgr->saveCurrentScene(path.c_str());
                    MSceneIO::setCurrentPath(path.c_str()); // keep MSceneIO in sync
                    saved = true;
                }
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::PopStyleColor();
    return saved;
}

// ── File / Save Scene ─────────────────────────────────────────────────────────

static constexpr const char* SAVE_DIALOG_KEY    = "SaveSceneDlg";
static bool s_saveWantOpen = false;

bool MSaveSceneMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MSaveSceneMenubarItem());
    return true;
}();

int MSaveSceneMenubarItem::getPriority() const { return MMenubarItem::PRIORITY_HIGHEST + 2; }
SString MSaveSceneMenubarItem::getPath()  const { return "File/Save Scene"; }

void MSaveSceneMenubarItem::onSelect()
{
    SString path = resolveCurrentScenePath();
    if (!path.empty())
    {
        // Known path — save immediately, no dialog needed.
        auto* mgr = dynamic_cast<MEditorSceneManager*>(
            MSceneManager::getSceneManagerInstance());
        if (mgr) mgr->saveCurrentScene(path);
    }
    else
    {
        // New unsaved scene — fall through to Save As dialog.
        s_saveWantOpen = true;
    }
}

void MSaveSceneMenubarItem::drawPopup()
{
    if (s_saveWantOpen)
    {
        s_saveWantOpen = false;
        openSaveDialog(SAVE_DIALOG_KEY);
    }
    drawSaveDialog(SAVE_DIALOG_KEY);
}

// ── File / Save Scene As ──────────────────────────────────────────────────────

static constexpr const char* SAVE_AS_DIALOG_KEY = "SaveSceneAsDlg";
static bool s_saveAsWantOpen = false;

bool MSaveSceneAsMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MSaveSceneAsMenubarItem());
    return true;
}();

int MSaveSceneAsMenubarItem::getPriority() const { return MMenubarItem::PRIORITY_HIGHEST + 3; }
SString MSaveSceneAsMenubarItem::getPath()  const { return "File/Save Scene As"; }

void MSaveSceneAsMenubarItem::onSelect()
{
    s_saveAsWantOpen = true;
}

void MSaveSceneAsMenubarItem::drawPopup()
{
    if (s_saveAsWantOpen)
    {
        s_saveAsWantOpen = false;
        // Pre-fill the filename from the current path if we have one.
        SString current = resolveCurrentScenePath();
        SString suggestion = current.empty()
            ? SString("untitled.scml")
            : SString(std::filesystem::path(current.str()).filename().string().c_str());
        openSaveDialog(SAVE_AS_DIALOG_KEY, suggestion);
    }
    drawSaveDialog(SAVE_AS_DIALOG_KEY);
}