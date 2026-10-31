//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "imgui.h"
#include "imgui_internal.h" // ImGuiDockNodeFlags_NoCloseButton
#include "imguisubwindow.h"
#include "imguisubwindowmanager.h"

MImGuiSubWindow::MImGuiSubWindow(int x, int y) {
    MImGuiSubWindowManager::add(this);
    ImGui::SetNextWindowSize(ImVec2(x, y), ImGuiCond_FirstUseEver);
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

void MImGuiSubWindow::draw(float deltaTime)
{
    if (!open)
        return;

    if (focusNext)
    {
        ImGui::SetNextWindowFocus();
        focusNext = false;
    }

    // The part after ### is the window's ImGui ID: its type name, which stays
    // the same if the title changes, so saved layouts keep finding it.
    // Each tab already has its own close (X) button, so hide the dock node's
    // extra one at the right end of the tab bar (it only closed the active tab).
    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoCloseButton;
    ImGui::SetNextWindowClass(&windowClass);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, minSize);
    const bool customPadding = windowPadding.x >= 0.0f && windowPadding.y >= 0.0f;
    if (customPadding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPadding);
    ImGui::Begin(SString::format("{0}###{1}", title.c_str(), typeInfo().name).c_str(), &open);
    // Padding is read by Begin(); pop it so child windows and popups opened
    // from this window keep the theme's padding.
    if (customPadding)
        ImGui::PopStyleVar();

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        handleInput(deltaTime);

    onGui(deltaTime);
    ImGui::End();
    ImGui::PopStyleVar();
}

void MImGuiSubWindow::handleInput(float deltaTime)
{

}

SString MImGuiSubWindow::getTitle() const {
    return title;
}