//
// Created by Vishnu Rajendran on 2024-09-24.
//
#include "imgui.h"
#include "imgui-SFML.h"
#include "imguiwindow.h"
#include "imguisubwindow.h"
#include "imguisubwindowmanager.h"
#include "imguiwindowconstants.h"

MImGuiWindow::MImGuiWindow(const SString &title) : MWindow(title) {

}

MImGuiWindow::MImGuiWindow(const SString &title, int sizeX, int sizeY, int fps) : MWindow(title, sizeX, sizeY, fps) {
    coreWindow.create(sf::VideoMode::getDesktopMode(), title.str(), sf::Style::Default);
    ImGui::SFML::Init(coreWindow);
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 2.0f;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);  // Full-screen size
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
}

void MImGuiWindow::clear() {
    MWindow::clear();
}

void MImGuiWindow::update() {
    while (coreWindow.pollEvent(event)) {
        ImGui::SFML::ProcessEvent(coreWindow, event);

        if (event.type == sf::Event::Closed) {
            coreWindow.close();
        }
    }
    ImGui::SFML::Update(coreWindow, deltaClock.restart());
    showDockSpace();
    drawMenuBar();
    drawImGuiSubWindows();
    coreWindow.clear();
    ImGui::SFML::Render(coreWindow);
    coreWindow.display();
}

void MImGuiWindow::close() {
    ImGui::SFML::Shutdown();
    MWindow::close();
}

void MImGuiWindow::drawImGuiSubWindows() {
    for(auto window : MImGuiSubWindowManager::getSubWindows()){
        if(window)
            window->draw();
    }
}

void MImGuiWindow::showDockSpace() {
    ImGuiIO& io = ImGui::GetIO();

    // Create a full-screen window with no decoration or background
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);  // Full-screen size
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

    // Set window flags for no decoration, background, or inputs
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    // Open the window
    ImGui::Begin(MAIN_DOCKSPACE_ID.c_str(), nullptr, windowFlags);

    // Create the dock space
    ImGuiID dockspaceID = ImGui::GetID(MAIN_DOCKSPACE_ID.c_str());
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

void MImGuiWindow::drawMenuBar(){
    if(ImGui::BeginMainMenuBar()){
        //Todo: Add Menu Items here
        if(ImGui::BeginMenu("File")){
           ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Edit")){
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Help")){
            ImGui::EndMenu();
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Meteorite Editor  ").x - ImGui::GetStyle().FramePadding.x);
        ImGui::Text("Meteorite Editor");
        ImGui::EndMainMenuBar();
    }
}


