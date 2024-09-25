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
    ImGuiIO& io = ImGui::GetIO();

    io.FontGlobalScale = 2.5f;
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

    drawMenuBar();
    showDockSpace();
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
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    // Create a full-screen window with no decoration or background
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Set window flags for no decoration, background, or inputs
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 40.0f));
    ImGui::Begin(MAIN_DOCKSPACE_ID.c_str(), nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    // Create the dock space
    ImGuiID dockspaceID = ImGui::GetID(MAIN_DOCKSPACE_ID.c_str());
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
}

void MImGuiWindow::drawMenuBar(){
    if(ImGui::BeginMainMenuBar()){
        //Todo: Add Menu Items here
        if(ImGui::BeginMenu("File")){
            if(ImGui::MenuItem("Log")){
                MLOG(TEXT("Log Clicked"));
            }
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

void MImGuiWindow::loadFontFile(const SString &pathToFile, float pointSize) {
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(pathToFile.c_str(), pointSize);
    if (!font) {
        MERROR(TEXT("Unable to load font : ")+pathToFile);
        return;
    }
    ImGui::SFML::UpdateFontTexture();
    ImGui::GetIO().FontDefault = font;
}


