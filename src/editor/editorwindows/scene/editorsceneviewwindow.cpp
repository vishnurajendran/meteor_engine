//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "SFML/OpenGL.hpp"
#include "editorsceneviewwindow.h"

MEditorSceneViewWindow::MEditorSceneViewWindow(): MEditorSceneViewWindow(700, 300) {

}

MEditorSceneViewWindow::MEditorSceneViewWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Scene";
    renderTexture.create(1920, 1080);
    glViewport(0,0,1920, 1080);
    MGraphicsRenderer::initialise(&renderTexture);
}

void MEditorSceneViewWindow::onGui() {
    auto contentRegion = ImGui::GetContentRegionAvail();
    if(renderTexture.getSize().x != contentRegion.x || renderTexture.getSize().y != contentRegion.y) {
        renderTexture.create(contentRegion.x, contentRegion.y);
    }
    ImGui::Image(renderTexture, contentRegion);
}