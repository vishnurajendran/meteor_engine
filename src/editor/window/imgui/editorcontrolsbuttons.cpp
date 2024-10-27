//
// Created by ssj5v on 26-10-2024.
//

#include "editorcontrolsbuttons.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include "core/utils/dpihelper.h"

sf::Texture MEditorControlsButtons::playTexture;

void MEditorControlsButtons::playButton() {
    const SString METEOR_PLAYER_PROC_NAME = "meteor_player.exe";
    const SString METEOR_PLAYER_ICO_PATH = "meteor_assets/icons/play.png";
    if(playTexture.getSize().x <= 0) {
        playTexture.loadFromFile(METEOR_PLAYER_ICO_PATH.c_str());
    }

    auto windowSize = ImGui::GetWindowSize();
    ImGui::SetCursorPosX(windowSize.x/2 - playTexture.getSize().x/2);
    if(ImGui::ImageButton("##MTR_PLAY_BTTN", playTexture, ImVec2(playTexture.getSize().x, playTexture.getSize().y))) {
        std::system(METEOR_PLAYER_PROC_NAME.c_str());
    }
}

void MEditorControlsButtons::runtimeControls() {
    playButton();
}
