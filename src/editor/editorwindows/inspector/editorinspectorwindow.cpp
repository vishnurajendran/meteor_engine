//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "editorinspectorwindow.h"
#include "editor/app/editorapplication.h"
#include "misc/cpp/imgui_stdlib.h"

MEditorInspectorWindow::MEditorInspectorWindow(): MEditorInspectorWindow(700, 300) {

}

MEditorInspectorWindow::MEditorInspectorWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Inspector";
}

void MEditorInspectorWindow::onGui() {
    if(MEditorApplication::Selected == nullptr)
        return;

    auto selected = MEditorApplication::Selected;
    ImGui::Text("Name:");
    ImGui::SameLine();
    auto size = ImGui::GetContentRegionAvail();
    ImGui::SetNextItemWidth(size.x - 20);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
    std::string name = selected->getName();
    if(ImGui::InputText("##EntityNameField", &name,ImGuiInputTextFlags_EnterReturnsTrue)) {
       selected->setName(name);
    }
}