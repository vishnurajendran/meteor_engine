//
// Created by Vishnu Rajendran on 2024-09-25.
//
#include "imgui.h"

#include "editorsceneviewwindow.h"
#include "core/engine/camera/camera.h"
#include "core/engine/camera/viewmanagement.h"
#include "editor/app/editorapplication.h"

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
    const auto& sizeChanged = renderTexture.getSize().x != contentRegion.x || renderTexture.getSize().y != contentRegion.y;
    if(sizeChanged) {
        renderTexture.create(contentRegion.x, contentRegion.y, settings);
    }
    ImGui::Image(renderTexture, contentRegion);
    sceneImageCursorPosition = ImGui::GetItemRectMin();

    drawTranformHandles();
    drawTransformGizmoSelector();
    drawModeSelector();
}

void MEditorSceneViewWindow::drawTransformGizmoSelector() {

    if(translateIcon.getSize().x <= 0) {
        translateIcon.loadFromFile("meteor_assets/icons/gizmo_translate.png");
    }

    if(rotateIcon.getSize().x <= 0) {
        rotateIcon.loadFromFile("meteor_assets/icons/gizmo_rotate.png");
    }

    if(scaleIcon.getSize().x <= 0) {
        scaleIcon.loadFromFile("meteor_assets/icons/gizmo_scale.png");
    }

    auto drawPos = ImVec2(sceneImageCursorPosition.x +10, sceneImageCursorPosition.y + 10);
    ImGui::SetCursorScreenPos(drawPos);
    auto selectionText = getCurrentTransformGizmoText();
    auto selectionTextSize = ImGui::CalcTextSize(selectionText.c_str()).x;
    auto seperatorText = STR(" | ");
    auto seperatorTextSize = ImGui::CalcTextSize(seperatorText.c_str()).x;

    ImGui::BeginChild("##MTR_TransformationGizmos", ImVec2(130 + seperatorTextSize + selectionTextSize, 37.5), true, ImGuiChildFlags_Border);

    // Draw opaque background
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 min = ImGui::GetWindowPos();
    ImVec2 max = ImVec2(min.x + ImGui::GetWindowWidth(), min.y + ImGui::GetWindowHeight());

    drawList->AddRectFilled(min, max, IM_COL32(30, 30, 30, 255)); // solid color (RGBA)


    ImGui::SameLine();
    ImGui::Text(selectionText.c_str());
    ImGui::SameLine();
    ImGui::Text(seperatorText.c_str());
    ImGui::SameLine();

    if(ImGui::ImageButton("##SLCT_GIZMO_TRANSLATE", translateIcon, ImVec2(translateIcon.getSize().x, translateIcon.getSize().y))) {
        transformOperation = ImGuizmo::TRANSLATE;
    }

    ImGui::SameLine();
    if(ImGui::ImageButton("##SLCT_GIZMO_ROTATE", rotateIcon, ImVec2(rotateIcon.getSize().x, rotateIcon.getSize().y))) {
        transformOperation = ImGuizmo::ROTATE;
    }

    ImGui::SameLine();
    if(ImGui::ImageButton("##SLCT_GIZMO_SCALE", scaleIcon, ImVec2(scaleIcon.getSize().x, scaleIcon.getSize().y))) {
        transformOperation = ImGuizmo::SCALE;
    }
    ImGui::EndChild();
}

void MEditorSceneViewWindow::drawModeSelector() {
    if(localSpaceIcon.getSize().x <= 0) {
        localSpaceIcon.loadFromFile("meteor_assets/icons/gizmo_local.png");
    }

    if(worldSpaceIcon.getSize().x <= 0) {
        worldSpaceIcon.loadFromFile("meteor_assets/icons/gizmo_world.png");
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
    auto selectionText = getCurrentTransformModeText();
    auto selectionTextSize = ImGui::CalcTextSize(selectionText.c_str()).x;
    auto seperatorText = STR(" | ");
    auto seperatorTextSize = ImGui::CalcTextSize(seperatorText.c_str()).x;

    ImGui::SameLine();
    ImGui::BeginChild("##MTR_TransformationMode", ImVec2(95 + seperatorTextSize + selectionTextSize, 37.5), true, ImGuiChildFlags_Border);
    // Draw opaque background
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 min = ImGui::GetWindowPos();
    ImVec2 max = ImVec2(min.x + ImGui::GetWindowWidth(), min.y + ImGui::GetWindowHeight());
    drawList->AddRectFilled(min, max, IM_COL32(30, 30, 30, 255)); // solid color (RGBA)
    
    ImGui::SameLine();
    ImGui::Text(selectionText.c_str());
    ImGui::SameLine();
    ImGui::Text(seperatorText.c_str());
    ImGui::SameLine();

    if(ImGui::ImageButton("##SLCT_MODE_LOCAL", localSpaceIcon, ImVec2(localSpaceIcon.getSize().x, localSpaceIcon.getSize().y))) {
        transformMode = ImGuizmo::LOCAL;
    }

    ImGui::SameLine();
    if(ImGui::ImageButton("##SLCT_MODE_WORLD", worldSpaceIcon, ImVec2(worldSpaceIcon.getSize().x, worldSpaceIcon.getSize().y))) {
        transformMode = ImGuizmo::WORLD;
    }
    ImGui::EndChild();
}

void MEditorSceneViewWindow::drawTranformHandles() {
    if(!MEditorApplication::Selected)
        return;

    auto cameras = MViewManagement::getCameras();
    if(cameras.size() < 0)
        return;

    auto primaryCamera = cameras[0];
    if(!primaryCamera)
        return;

    auto selected = MEditorApplication::Selected;
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    auto width = renderTexture.getSize().x;
    auto height = renderTexture.getSize().y;
    ImGuizmo::SetRect(sceneImageCursorPosition.x, sceneImageCursorPosition.y , width, height);

    auto viewMat = primaryCamera->getViewMatrix();
    auto projMat = primaryCamera->getProjectionMatrix(SVector2(renderTexture.getSize().x, renderTexture.getSize().y));
    auto transform =  selected->getTransformMatrix();
    ImGui::SetItemAllowOverlap();
    ImGuizmo::Manipulate(glm::value_ptr(viewMat), glm::value_ptr(projMat),transformOperation,
    transformMode, glm::value_ptr(transform));

    if(!ImGuizmo::IsUsing()) {
        return;
    }

    SVector3 newWorldPos;
    SVector3 newWorldScale;
    SVector3 newWorldRotation;

    SVector3 cumulativeParentScale(1.0f); // Start with a uniform scale of 1
    auto currentParent = selected->getParent();
    while (currentParent) {
        cumulativeParentScale *= currentParent->getRelativeScale();
        currentParent = currentParent->getParent();
    }

    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), glm::value_ptr(newWorldPos),
        glm::value_ptr(newWorldRotation), glm::value_ptr(newWorldScale));

    selected->setWorldRotation(eulerToQuaternion(newWorldRotation));
    selected->setWorldPosition(newWorldPos);

    if(selected->getParent()) {
        selected->setRelativeScale(newWorldScale / cumulativeParentScale);
    }
    else
        selected->setRelativeScale(newWorldScale);
}

SString MEditorSceneViewWindow::getCurrentTransformGizmoText() const {
    switch(transformOperation) {
        case ImGuizmo::OPERATION::TRANSLATE:
            return "Translate";
        case ImGuizmo::OPERATION::ROTATE:
            return "Rotation";
        case ImGuizmo::OPERATION::SCALE:
            return "Scale";
        default:
            return "???";
    }
}

SString MEditorSceneViewWindow::getCurrentTransformModeText() const {
    switch(transformMode) {
        case ImGuizmo::MODE::LOCAL:
            return "Local";
        case ImGuizmo::MODE::WORLD:
            return "World";
        default:
            return "???";
    }
}
