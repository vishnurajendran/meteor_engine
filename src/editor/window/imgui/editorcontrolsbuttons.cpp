//
// Created by ssj5v on 26-10-2024.
//

#include "editorcontrolsbuttons.h"
#include "../../../../cmake-build-player_debug/_deps/imgui-src/imgui.h"
#include "../../../../cmake-build-player_debug/_deps/imgui-sfml-src/imgui-SFML.h"

sf::Texture MEditorControlsButtons::playTexture;

void MEditorControlsButtons::playButton() {
    const SString METEOR_PLAYER_PROC_NAME = "meteor_player.exe";
    const SString METEOR_PLAYER_ICO_PATH = "meteor_assets/icons/play.png";
    if(playTexture.getSize().x <= 0) {
        playTexture.loadFromFile(METEOR_PLAYER_ICO_PATH.c_str());
    }

    ImGui::SameLine(ImGui::GetWindowWidth() / 2);
    auto size = playTexture.getSize();
    if(ImGui::ImageButton("#MTR_PLAY_BTTN", playTexture, ImVec2(size.x, size.y))) {
        std::system(METEOR_PLAYER_PROC_NAME.c_str());
    }
}
