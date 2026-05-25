//
// Created by ssj5v on 26-10-2024.
//

#include "editorcontrolsbuttons.h"
#include "core/utils/dpihelper.h"
#include "default_engine_icon_paths.h"
#include "editor/app/editorapplication.h"
#include "imgui-SFML.h"
#include "imgui.h"

sf::Texture MEditorControlsButtons::playTexture;
sf::Texture MEditorControlsButtons::stopTexture;

void MEditorControlsButtons::simulationStartButton()
{

    const SString METEOR_PLAYER_ICO_PATH = SEditorAssetPaths::LOWRES_TEX_BTTN_PLAY;
    if (playTexture.getSize().x <= 0)
    {
        playTexture.loadFromFile(METEOR_PLAYER_ICO_PATH.c_str());
    }

    auto windowSize = ImGui::GetWindowSize();
    ImGui::SetCursorPosX(windowSize.x / 2 - playTexture.getSize().x / 2);

    if (ImGui::ImageButton("##MTR_SIM_START_BTTN", playTexture, ImVec2(playTexture.getSize().x, playTexture.getSize().y)))
    {
        if (auto* editorApp = dynamic_cast<MEditorApplication*>(MApplication::getAppInstance()))
            editorApp->startSimulation();
    }
}

void MEditorControlsButtons::simulationStopButton()
{
    const SString METEOR_PLAYER_ICO_PATH = SEditorAssetPaths::LOWRES_TEX_BTTN_STOP;
    if (stopTexture.getSize().x <= 0)
    {
        stopTexture.loadFromFile(METEOR_PLAYER_ICO_PATH.c_str());
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
    if (!editorApp->isPlaying())
        simulationStartButton();
    else
        simulationStopButton();
}
