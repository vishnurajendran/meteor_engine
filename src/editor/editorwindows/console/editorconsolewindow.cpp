//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editor/meteorite.h"
#include "editorconsolewindow.h"

MEditorConsoleWindow::MEditorConsoleWindow() :MEditorConsoleWindow(700, 300) {
    subscriptionId = MLogger::subscribe(std::bind(&MEditorConsoleWindow::onLogRecieved, this, std::placeholders::_1));
}

MEditorConsoleWindow::MEditorConsoleWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Console";
}

MEditorConsoleWindow::~MEditorConsoleWindow() {
    MLogger::unsubscribe(subscriptionId);
}

void MEditorConsoleWindow::onLogRecieved(SString log) {
    logs.push_back(log);
}

void MEditorConsoleWindow::onGui() {

    if(ImGui::Button("Clear")){
        logs.clear();
    }

    ImVec2 availableSpace = ImGui::GetContentRegionAvail();
    if(ImGui::BeginListBox("##logs", availableSpace)){
       for(const auto& log : logs){
           ImGui::PushID(&log);  // Use the item's memory address as a unique ID
           ImGui::Selectable(log.c_str());
           ImGui::PopID();
       }
       ImGui::EndListBox();
    }
}
