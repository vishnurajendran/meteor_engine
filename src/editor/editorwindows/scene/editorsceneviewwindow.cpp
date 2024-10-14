//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "editorsceneviewwindow.h"

MEditorSceneViewWindow::MEditorSceneViewWindow(): MEditorSceneViewWindow(700, 300) {

}

MEditorSceneViewWindow::MEditorSceneViewWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Scene";
    settings.depthBits = 24;

    renderTexture.create(1920, 1080, settings);
    MGraphicsRenderer::initialise(&renderTexture);
}

void MEditorSceneViewWindow::onGui() {
    auto contentRegion = ImGui::GetContentRegionAvail();
    if(renderTexture.getSize().x != contentRegion.x || renderTexture.getSize().y != contentRegion.y) {
        renderTexture.create(contentRegion.x, contentRegion.y, settings);
    }
    ImGui::Image(renderTexture, contentRegion);
}