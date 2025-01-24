//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "editorhierarchywindow.h"
#include "editor/app/editorapplication.h"

MEditorHierarchyWindow::MEditorHierarchyWindow(): MEditorHierarchyWindow(700, 300) {

}

MEditorHierarchyWindow::MEditorHierarchyWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Hierarchy";
    sceneTex.loadFromFile("meteor_assets/icons/scene.png");
    entityTex.loadFromFile("meteor_assets/icons/spatial.png");

    auto dpi = DPIHelper::GetDPIScaleFactor();
    sceneTexSize = sf::Vector2f(sceneTex.getSize().x * dpi,sceneTex.getSize().y * dpi);
    entityTexSize = sf::Vector2f(entityTex.getSize().x * dpi,entityTex.getSize().y * dpi);
}

void MEditorHierarchyWindow::onGui() {
    // Create a child window with a specific size and enable scrolling
    auto size = ImGui::GetContentRegionAvail();

    auto scene = MSceneManager::getSceneManagerInstance()->getActiveScene();

    if(!scene)
        return;
    SString hidden = "##" + scene->getName().str();
    ImGui::BeginChild(getGUID().c_str(), size, true, ImGuiWindowFlags_HorizontalScrollbar);
    // Root node

    ImGui::PushID(scene->getGUID().c_str());
    bool open = ImGui::TreeNodeEx(hidden.c_str(),  ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::SameLine();
    ImGui::Image(sceneTex, sceneTexSize);
    ImGui::SameLine();
    ImGui::Text(scene->getName().c_str());
    const auto& rootEntities = scene->getRootEntities();
    if (open) {
        for(const auto& rootObjs : rootEntities) {
            drawRecursiveSceneTree(rootObjs, 1);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
    ImGui::EndChild();
}

void MEditorHierarchyWindow::drawRecursiveSceneTree(MSpatialEntity* spatial, int depth) {
    if(!spatial)
        return;

    //ignore hidden entities.
    if ((spatial->getEntityFlags() & EEntityFlags::HideInEditor) == EEntityFlags::HideInEditor)
    {
        return;
    }

    //draw leaf
    if(spatial->getChildren().empty()) {
        drawLeaf(spatial);
        return;
    }

    //draw current node
    ImGui::PushID(spatial->getGUID().c_str());
    auto hidden = STR("##"+spatial->getGUID().str());

    bool open = ImGui::TreeNodeEx(hidden.c_str());
    ImGui::SameLine();
    ImGui::Image(entityTex, entityTexSize);
    ImGui::SameLine();
    if(ImGui::Selectable(spatial->getName().c_str(), MEditorApplication::Selected == spatial)) {
        MEditorApplication::Selected = spatial;
    }

    if (open) {
        for(auto child : spatial->getChildren()) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3*depth);
            drawRecursiveSceneTree(child, depth+1);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void MEditorHierarchyWindow::drawLeaf(MSpatialEntity *spatial) {

    if(!spatial)
        return;

    auto hidden = "##"+spatial->getGUID().str();
    ImGui::PushID(spatial->getGUID().c_str());
    if(ImGui::Selectable( hidden.c_str(), MEditorApplication::Selected == spatial)) {
        MEditorApplication::Selected = spatial;
    }
    ImGui::SameLine();
    ImGui::Image(entityTex, entityTexSize);
    ImGui::SameLine();
    ImGui::Text("%s", spatial->getName().c_str());
    ImGui::PopID();
}

