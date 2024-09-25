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

void MImGuiSubWindow::draw() {
    if(ImGui::Begin(title.c_str())){
        onGui();
        ImGui::End();
    }
}

SString MImGuiSubWindow::getTitle() const {
    return title;
}
