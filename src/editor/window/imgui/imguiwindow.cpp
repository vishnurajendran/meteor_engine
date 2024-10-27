//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include <GL/glew.h>
#include "imgui.h"
#include "imgui-SFML.h"
#include "imguiwindow.h"

#include "editorcontrolsbuttons.h"
#include "imguisubwindow.h"
#include "imguisubwindowmanager.h"
#include "imguiwindowconstants.h"
#include "imguistyles.h"
#include "imgui_internal.h"

MImGuiWindow::MImGuiWindow(const SString &title) : MImGuiWindow(title, 800, 600, 60) {

}

MImGuiWindow::MImGuiWindow(const SString &title, int sizeX, int sizeY, int fps) : MWindow(title, sizeX, sizeY, fps) {
    this->targetFPS = fps;
    sf::ContextSettings settings;
    settings.majorVersion = 4;
    settings.minorVersion = 6;
    settings.depthBits = 24;

    coreWindow.create(sf::VideoMode::getDesktopMode(), title.str(), sf::Style::Default, settings);
    coreWindow.setFramerateLimit(fps);
    coreWindow.setActive(true);

    if(!ImGui::SFML::Init(coreWindow)) {
        MERROR(STR("Failed to initialize SFML" ));
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    loadFontFile("meteor_assets/fonts/Open-sans/OpenSansEmoji.ttf", 18*DPIHelper::GetDPIScaleFactor());

    io.FontGlobalScale = 1;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    deepDarkTheme();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);  // Full-screen size
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

    MGraphicsRenderer::initialise(&coreWindow);
}

void MImGuiWindow::clear() {
    MWindow::clear();
}

void MImGuiWindow::update() {
    const sf::Time frameTime = sf::seconds(1.f / targetFPS);
    while (coreWindow.pollEvent(event)) {
        ImGui::SFML::ProcessEvent(coreWindow, event);
        if (event.type == sf::Event::Closed) {
            close();
            return;
        }
    }

    draw();

    sf::Time elapsed = clock.getElapsedTime();
    sf::Time sleepTime = frameTime - elapsed;
    if (sleepTime > sf::Time::Zero) {
        sf::sleep(sleepTime);  // Sleep to limit the framerate
    }
    clock.restart();
}

void MImGuiWindow::drawGUI() {
    ImGui::SFML::Update(coreWindow, deltaClock.restart());
    drawMenuBar();
    showDockSpace();
    drawImGuiSubWindows();
    ImGui::SFML::Render(coreWindow);
}

void MImGuiWindow::draw() {
    MGraphicsRenderer::draw();
    drawGUI();
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
           ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Edit")){
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Help")){
            ImGui::EndMenu();
        }

        drawControls();

        ImGui::EndMainMenuBar();
    }
}

void MImGuiWindow::drawControls() {
    MEditorControlsButtons::runtimeControls();
}

void MImGuiWindow::loadFontFile(const SString &pathToFile, float pointSize) {
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(pathToFile.c_str(), pointSize);
    if (!font) {
        MERROR(STR("Unable to load font : ") + pathToFile);
        return;
    }

    if(!ImGui::SFML::UpdateFontTexture()) {
        MWARN("Unable to load font file");
    }
    ImGui::GetIO().FontDefault = font;
}


