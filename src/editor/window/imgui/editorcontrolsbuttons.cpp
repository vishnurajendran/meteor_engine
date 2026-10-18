//
// Created by ssj5v on 26-10-2024.
//

#include "editorcontrolsbuttons.h"
#include "default_engine_icon_paths.h"
#include "editor/app/editorapplication.h"
#include "editor/editor_utils/engine_textures.h"
#include "imgui-SFML.h"
#include "imgui.h"

sf::Texture MEditorControlsButtons::playTexture;
sf::Texture MEditorControlsButtons::stopTexture;

void MEditorControlsButtons::simulationStartButton()
{

    // Try once; a missing icon should not retry (and warn) every frame.
    static bool playTried = false;
    if (!playTried)
    {
        playTried = true;
        loadEngineTexture(playTexture, SEngineAssetIconPaths::LOWRES_TEX_BTTN_PLAY);
    }

    auto windowSize = ImGui::GetWindowSize();
    ImGui::SetCursorPosX(windowSize.x / 2 - playTexture.getSize().x / 2);

    // Disabled while scripts compile or have errors; the tooltip says which.
    auto* editorApp = dynamic_cast<MEditorApplication*>(MApplication::getAppInstance());
    SString blockedReason;
    const bool canPlay = !editorApp || editorApp->canStartSimulation(blockedReason);

    ImGui::BeginDisabled(!canPlay);
    if (ImGui::ImageButton("##MTR_SIM_START_BTTN", playTexture, ImVec2(playTexture.getSize().x, playTexture.getSize().y)))
    {
        if (editorApp)
            editorApp->startSimulation();
    }
    ImGui::EndDisabled();

    if (!canPlay && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("%s", blockedReason.c_str());
}

void MEditorControlsButtons::simulationStopButton()
{
    static bool stopTried = false;
    if (!stopTried)
    {
        stopTried = true;
        loadEngineTexture(stopTexture, SEngineAssetIconPaths::LOWRES_TEX_BTTN_STOP);
    }

    auto windowSize = ImGui::GetWindowSize();
    ImGui::SetCursorPosX(windowSize.x / 2 - stopTexture.getSize().x / 2);

    if (ImGui::ImageButton("##MTR_SIM_STOP_BTTN", stopTexture, ImVec2(stopTexture.getSize().x, stopTexture.getSize().y)))
    {
        if (auto* editorApp = dynamic_cast<MEditorApplication*>(MApplication::getAppInstance()))
            editorApp->stopSimulation();
    }
}

void MEditorControlsButtons::runtimeControls() {
    auto* editorApp = dynamic_cast<MEditorApplication*>(MApplication::getAppInstance());
    if (!editorApp->isSimulating())
        simulationStartButton();
    else
        simulationStopButton();
}
