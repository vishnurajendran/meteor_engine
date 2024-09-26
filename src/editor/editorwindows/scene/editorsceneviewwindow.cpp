//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "SFML/OpenGL.hpp"
#include "editorsceneviewwindow.h"

MEditorSceneViewWindow::MEditorSceneViewWindow(): MEditorSceneViewWindow(700, 300) {

}

MEditorSceneViewWindow::MEditorSceneViewWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Scene";
    resolutionsText = {"1280x720", "1920x1080"};
    resolutionsData = {{1280, 720},{1920,1080}};

    renderTexture.create(1920, 1080);
    currentItm = 1;
    glViewport(0,0,1920, 1080);
    MGraphicsRenderer::initialise(&renderTexture);
}

void MEditorSceneViewWindow::onGui() {
    ImGui::Text("Resolution : ");
    ImGui::SameLine();
    if(ImGui::BeginTabBar("##sceneTabBar")){
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300.0f);
        if(ImGui::BeginCombo("##sceneCombo", resolutionsText[currentItm])){
            for(int i = 0;i<resolutionsText.size();i++){
                if(ImGui::Selectable(resolutionsText[i], i == currentItm)){
                    currentItm = i;
                    auto res = resolutionsData[currentItm];
                    renderTexture.create(std::get<0>(res), std::get<1>(res));
                    MLOG(TEXT("Scene Window Resolution Updated"));
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::Text(" |  Scale : ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300.0f);
        ImGui::SliderFloat("##scale", &scale, 0.5f, 3.0f);

        ImGui::EndTabBar();
    }

    auto contentRegion = ImGui::GetContentRegionAvail();
    auto imgSize = renderTexture.getSize();

    float xOffset = (contentRegion.x - imgSize.x * scale) * 0.5f;
    float yOffset = (contentRegion.y - imgSize.y * scale) * 0.5f;
    if (xOffset > 0.0f) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
    }
    if (yOffset > 0.0f) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
    }
    ImGui::Image(renderTexture, ImVec2(imgSize.x * scale, imgSize.y * scale));
}