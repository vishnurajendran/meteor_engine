//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "imgui.h"
#include "imguisubwindow.h"
#include "imguisubwindowmanager.h"

MImGuiSubWindow::MImGuiSubWindow(int x, int y) {
    MImGuiSubWindowManager::add(this);
    ImGui::SetNextWindowSize(ImVec2(x, y));
}

MImGuiSubWindow::MImGuiSubWindow(const SString &title, int x, int y) : MImGuiSubWindow(x, y) {
    this->title = title;

}

MImGuiSubWindow::~MImGuiSubWindow() {
    MImGuiSubWindowManager::remove(this);
}

void MImGuiSubWindow::setWindowConstraints(float minx, float miny, float maxx, float maxy) {
    minSize = ImVec2(minx, miny);
    maxSize = ImVec2(maxx, maxy);
}

void MImGuiSubWindow::draw() {
    ImGui::Begin(title.c_str());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, minSize);
    onGui();
    ImGui::PopStyleVar();
    ImGui::End();
}

SString MImGuiSubWindow::getTitle() const {
    return title;
}
