//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorconsolewindow.h"
#include "editor/window/imgui/imguisubwindow.h"

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
           auto hidden= STR("##") + log;
           auto tag = log.substring(0, 3);
           auto msg = log.substring(5, log.length());

           ImGui::PushID(&log);
           if(ImGui::Selectable(hidden.c_str(), selectedIndx == &log)){
               selectedIndx = &log;
           }
           ImGui::PopID();

           auto color = getLabelColor(tag);
           ImGui::SameLine();
           ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.r, color.g, color.b, color.a));
           ImGui::Text("[%s]:", tag.c_str());
           ImGui::PopStyleColor();
           ImGui::SameLine();
           ImGui::Text("%s", msg.c_str());

       }
       ImGui::EndListBox();
    }
}

SColor MEditorConsoleWindow::getLabelColor(SString label) {
    if(label == "LOG")
        return SColor::skyBlue();
    else if(label == "WRN")
        return SColor::yellow();
    else if(label == "ERR")
        return SColor::red();

    return {1,1,1,1};
}
