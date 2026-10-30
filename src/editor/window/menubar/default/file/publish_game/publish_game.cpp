//
// publish_game.cpp
//

#include "publish_game.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "core/application/application.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/scene/scenemanager.h"
#include "core/utils/logger.h"
#include "editor/app/editorapplication.h"
#include "editor/editor_utils/editor_utility.h"
#include "editor/editorscenemanager/editorscenemanager.h"
#include "editor/publishing/game_publisher.h"
#include "editor/window/menubar/menubartree.h"
#include "core/memory/mnew.h"

namespace
{
    constexpr const char* SETTINGS_POPUP   = "##publish_game_dlg";
    constexpr const char* PROGRESS_POPUP   = "##MTR_PUBLISHING";
    constexpr const char* FOLDER_DIALOG    = "PublishOutputFolderDlg";
    constexpr float       LABEL_W          = 130.0f;

    bool wantOpenSettings = false;

    // Dialog state, filled from the .mtproj each time the dialog opens.
    char outputBuffer[1024]   = {};
    char nameBuffer[256]      = {};
    char versionBuffer[64]    = {};
    char publisherBuffer[256] = {};
    int  maxPakSizeMB         = 512;
    bool saveSceneFirst       = true;
    bool cancelPressed        = false;
    std::vector<SString> scenePaths;
    int  sceneIndex           = -1;
    SString dialogError;

    void copyToBuffer(char* buffer, size_t size, const SString& value)
    {
        std::strncpy(buffer, value.c_str(), size - 1);
        buffer[size - 1] = '\0';
    }

    MEditorApplication* editorApp()
    {
        return dynamic_cast<MEditorApplication*>(MApplication::getAppInstance());
    }

    SString formatSize(uint64_t bytes)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
        return SString(buf);
    }

    void labelledRow(const char* label)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(LABEL_W);
        ImGui::SetNextItemWidth(-1.f);
    }
}

bool MPublishGameMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(mnew<MPublishGameMenubarItem>());
    return true;
}();

// ---------------------------------------------------------------------------
// Menu item
// ---------------------------------------------------------------------------

void MPublishGameMenubarItem::onSelect()
{
    const auto* app = MApplication::getAppInstance();
    if (app && (app->isSimulating() || app->isPaused()))
    {
        MWARN("Publish Game is not available during play mode");
        return;
    }
    if (MGamePublisher::isPublishing())
        return;

    // Opened from drawPopup(), outside the menu bar's ImGui context.
    wantOpenSettings = true;
}

void MPublishGameMenubarItem::drawPopup()
{
    MGamePublisher::get().tick();

    if (wantOpenSettings)
    {
        wantOpenSettings = false;
        loadSettingsFromProject();
        ImGui::OpenPopup(SETTINGS_POPUP);
    }

    drawSettingsDialog();
    drawProgressPopup();
}

// ---------------------------------------------------------------------------
// Settings dialog
// ---------------------------------------------------------------------------

void MPublishGameMenubarItem::loadSettingsFromProject()
{
    dialogError = SString();
    auto* app = editorApp();
    if (!app) return;

    const auto& d = app->getProjectManager().getDescriptor();
    copyToBuffer(outputBuffer,    sizeof(outputBuffer),    d.publishOutputDirectory.empty() ? SString("Builds") : d.publishOutputDirectory);
    copyToBuffer(nameBuffer,      sizeof(nameBuffer),      d.name);
    copyToBuffer(versionBuffer,   sizeof(versionBuffer),   d.version);
    copyToBuffer(publisherBuffer, sizeof(publisherBuffer), d.publisher);
    maxPakSizeMB = d.maxPakSizeMB > 0 ? d.maxPakSizeMB : 512;

    // Every scene in the project, for the startup scene picker.
    scenePaths.clear();
    for (const auto& path : MAssetManager::getAssetPaths())
        if (path.endsWith(".scml") && !path.startsWith("meteor_assets/"))
            scenePaths.push_back(path);
    std::sort(scenePaths.begin(), scenePaths.end());

    sceneIndex = -1;
    for (int i = 0; i < static_cast<int>(scenePaths.size()); ++i)
        if (scenePaths[i] == d.startupScene)
            sceneIndex = i;
    if (sceneIndex < 0 && !scenePaths.empty())
        sceneIndex = 0;
}

void MPublishGameMenubarItem::drawSettingsDialog()
{
    const ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520, 0), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal(SETTINGS_POPUP, nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
        return;

    // -- Header ----------------------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.75f, 0.3f, 1.f));
    ImGui::TextUnformatted("Publish Game");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // -- Output folder ---------------------------------------------------------
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Output folder");
    ImGui::SameLine(LABEL_W);
    const float browseW = ImGui::CalcTextSize("Browse...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(-(browseW + ImGui::GetStyle().ItemSpacing.x));
    ImGui::InputText("##pg_output", outputBuffer, sizeof(outputBuffer));
    ImGui::SameLine();
    if (ImGui::Button("Browse..."))
    {
        IGFD::FileDialogConfig config;
        config.path              = ".";
        config.countSelectionMax = 1;
        config.flags             = ImGuiFileDialogFlags_Modal;
        // A null filter makes this a folder picker.
        ImGuiFileDialog::Instance()->OpenDialog(FOLDER_DIALOG, "Choose Output Folder", nullptr, config);
    }
    ImGui::TextDisabled("Relative paths are inside the project folder. The previous build there is replaced.");

    ImGui::Spacing();

    // -- Game info -------------------------------------------------------------
    labelledRow("Game name");
    ImGui::InputText("##pg_name", nameBuffer, sizeof(nameBuffer));
    labelledRow("Version");
    ImGui::InputText("##pg_version", versionBuffer, sizeof(versionBuffer));
    labelledRow("Publisher");
    ImGui::InputText("##pg_publisher", publisherBuffer, sizeof(publisherBuffer));

    // -- Startup scene ---------------------------------------------------------
    labelledRow("Startup scene");
    const char* preview = sceneIndex >= 0 ? scenePaths[sceneIndex].c_str() : "(no scenes in project)";
    if (ImGui::BeginCombo("##pg_scene", preview))
    {
        for (int i = 0; i < static_cast<int>(scenePaths.size()); ++i)
        {
            const bool selected = i == sceneIndex;
            if (ImGui::Selectable(scenePaths[i].c_str(), selected))
                sceneIndex = i;
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // -- Package ---------------------------------------------------------------
    labelledRow("Max pak size (MB)");
    if (ImGui::InputInt("##pg_paksize", &maxPakSizeMB, 64, 256))
        maxPakSizeMB = std::clamp(maxPakSizeMB, 16, 4096);

    ImGui::Spacing();
    ImGui::Checkbox("Save the open scene before publishing", &saveSceneFirst);

    // -- Error -----------------------------------------------------------------
    if (!dialogError.empty())
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.4f, 0.35f, 1.f));
        ImGui::PushTextWrapPos(ImGui::GetContentRegionMax().x);
        ImGui::TextUnformatted(dialogError.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    // -- Buttons ---------------------------------------------------------------
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float buttonW = 110.0f;
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - buttonW * 2.0f - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::Button("Publish", ImVec2(buttonW, 0)))
    {
        if (startPublish())
            ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(buttonW, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        ImGui::CloseCurrentPopup();

    // Folder picker, drawn on top of this dialog.
    if (ImGuiFileDialog::Instance()->Display(FOLDER_DIALOG, ImGuiWindowFlags_NoCollapse,
                                             ImVec2(600, 400), ImGui::GetIO().DisplaySize))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
            copyToBuffer(outputBuffer, sizeof(outputBuffer), SString(ImGuiFileDialog::Instance()->GetCurrentPath()));
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::EndPopup();
}

bool MPublishGameMenubarItem::startPublish()
{
    dialogError = SString();
    auto* app = editorApp();
    if (!app)
        return false;

    if (sceneIndex < 0 || sceneIndex >= static_cast<int>(scenePaths.size()))
    {
        dialogError = "The project has no scene to start with. Save a scene first.";
        return false;
    }

    SPublishSettings settings;
    settings.outputDirectory  = SString(outputBuffer);
    settings.gameName         = SString(nameBuffer);
    settings.version          = SString(versionBuffer);
    settings.publisher        = SString(publisherBuffer);
    settings.startupScenePath = scenePaths[sceneIndex];
    settings.maxPakSizeMB     = static_cast<uint32_t>(maxPakSizeMB);

    // Remember the settings in the .mtproj. The startup scene chosen here is
    // the project's startup scene.
    auto& d = app->getProjectManager().getMutableDescriptor();
    d.publishOutputDirectory = settings.outputDirectory;
    d.version                = settings.version;
    d.publisher              = settings.publisher;
    d.maxPakSizeMB           = maxPakSizeMB;
    d.startupScene           = settings.startupScenePath;
    app->getProjectManager().saveDescriptor();

    // Otherwise the build would contain the scene as last saved on disk.
    if (saveSceneFirst)
    {
        auto* scenes = dynamic_cast<MEditorSceneManager*>(MEngineSubsystemRegistry::getSubsystem<MSceneManager>());
        if (scenes && !scenes->getActiveScenePath().empty())
            scenes->saveCurrentScene(scenes->getActiveScenePath());
        else
            MWARN("Publish:: the open scene has never been saved, so it was not saved before publishing");
    }

    SString error;
    if (!MGamePublisher::get().start(settings, error))
    {
        dialogError = error;
        return false;
    }
    cancelPressed = false;
    return true;
}

// ---------------------------------------------------------------------------
// Progress popup (modal: blocks the editor while publishing)
// ---------------------------------------------------------------------------

void MPublishGameMenubarItem::drawProgressPopup()
{
    auto& publisher = MGamePublisher::get();
    const EPublishState state = publisher.getState();

    if (state != EPublishState::Idle && !ImGui::IsPopupOpen(PROGRESS_POPUP))
        ImGui::OpenPopup(PROGRESS_POPUP);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(440.0f, 0.0f));
    if (!ImGui::BeginPopupModal(PROGRESS_POPUP, nullptr,
                                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
        return;

    if (state == EPublishState::Idle)
    {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    const auto closeButton = [&publisher]()
    {
        ImGui::Spacing();
        const float w = 110.0f;
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - w);
        if (ImGui::Button("Close", ImVec2(w, 0)))
        {
            publisher.acknowledge();
            ImGui::CloseCurrentPopup();
        }
    };

    switch (state)
    {
        case EPublishState::Running:
        {
            const uint32_t done  = publisher.getDone();
            const uint32_t total = publisher.getTotal();
            const float fraction = total > 0 ? static_cast<float>(done) / static_cast<float>(total) : 0.0f;

            ImGui::TextUnformatted("Publishing...");
            ImGui::PushTextWrapPos(ImGui::GetContentRegionMax().x);
            ImGui::TextDisabled("%s", publisher.getCurrentItem().c_str());
            ImGui::PopTextWrapPos();

            const SString label = SString::format("{0} / {1}", SString::fromInt(static_cast<int>(done)),
                                                  SString::fromInt(static_cast<int>(total)));
            ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), label.c_str());

            ImGui::Spacing();
            const float w = 110.0f;
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - w);
            ImGui::BeginDisabled(cancelPressed);
            if (ImGui::Button(cancelPressed ? "Cancelling..." : "Cancel", ImVec2(w, 0)))
            {
                cancelPressed = true;
                publisher.cancel();
            }
            ImGui::EndDisabled();
            break;
        }

        case EPublishState::Succeeded:
        {
            const SPublishResult r = publisher.getResult();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.85f, 0.45f, 1.f));
            ImGui::TextUnformatted("Published");
            ImGui::PopStyleColor();
            ImGui::PushTextWrapPos(ImGui::GetContentRegionMax().x);
            ImGui::TextDisabled("%s", r.outputDirectory.c_str());
            ImGui::PopTextWrapPos();
            ImGui::Spacing();
            ImGui::Text("%d assets in %d pak file(s), %s", static_cast<int>(r.assetCount),
                        static_cast<int>(r.pakCount), formatSize(r.totalBytes).c_str());
            if (r.orphanMetaCount > 0)
                ImGui::TextDisabled("%d orphaned .meta file(s) skipped - see the console",
                                    static_cast<int>(r.orphanMetaCount));

            ImGui::Spacing();
            const float w = 110.0f;
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - w * 2.0f - ImGui::GetStyle().ItemSpacing.x);
            if (ImGui::Button("Open folder", ImVec2(w, 0)))
                MEditorUtility::openInFilExplorer(r.outputDirectory.str(), false);
            ImGui::SameLine();
            if (ImGui::Button("Close", ImVec2(w, 0)))
            {
                publisher.acknowledge();
                ImGui::CloseCurrentPopup();
            }
            break;
        }

        case EPublishState::Failed:
        {
            const SPublishResult r = publisher.getResult();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.4f, 0.35f, 1.f));
            ImGui::TextUnformatted("Publish failed");
            ImGui::PopStyleColor();
            ImGui::PushTextWrapPos(ImGui::GetContentRegionMax().x);
            ImGui::TextUnformatted(r.error.c_str());
            ImGui::PopTextWrapPos();
            ImGui::TextDisabled("The output folder was not changed.");
            closeButton();
            break;
        }

        case EPublishState::Cancelled:
            ImGui::TextUnformatted("Publish cancelled");
            ImGui::TextDisabled("The output folder was not changed.");
            closeButton();
            break;

        case EPublishState::Idle:
            break;
    }

    ImGui::EndPopup();
}
