//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include <GL/glew.h>
#include "imgui.h"
#include "imgui-SFML.h"
#include "imguiwindow.h"

#include "ImGuizmo.h"
#include "core/engine/gizmos/gizmos.h"
#include "editor/window/menubar/menubartree.h"
#include "editorcontrolsbuttons.h"
#include "imgui_internal.h"
#include "imguistyles.h"
#include "imguisubwindow.h"
#include "imguisubwindowmanager.h"
#include "imguiwindowconstants.h"

bool MImGuiWindow::initialiseWindow(const SString& inTitle, SVector2 inSize, int inFps)
{
    if (!MWindow::initialiseWindow(inTitle, inSize, inFps))
    {
        MERROR("Window::initialiseWindow Failed");
        return false;
    }

    if(!ImGui::SFML::Init(coreWindow)) {
        MERROR(STR("Failed to initialize SFML" ));
        return false;
    }

    if (!coreWindow.setActive(true))
    {
        MERROR(STR("Failed to activate Window Context" ));
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    loadFontFile("meteor_assets/fonts/Open-sans/OpenSansEmoji.ttf", 18*DPIHelper::GetDPIScaleFactor());

    io.FontGlobalScale = 1;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    applyDeepDarkTheme();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);  // Full-screen size
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

    return true;
}

void MImGuiWindow::clear() {
    MWindow::clear();
}

void MImGuiWindow::update(float deltaTime) {

    const sf::Time frameTime = sf::seconds(1.f / targetFPS);

    // event loop
    std::optional<sf::Event> event;
    while (event = coreWindow.pollEvent()) {
        ImGui::SFML::ProcessEvent(coreWindow, event.value());
        if (event.has_value() && event->is<sf::Event::Closed>()) {
            close();
            return;
        }
    }

    //Call the graphics systems to draw.
    if (graphicsFunction)
        graphicsFunction();

    drawGUI(deltaTime);
    coreWindow.display();

    // sync with target FPS
    const sf::Time elapsed = clock.getElapsedTime();
    const sf::Time sleepTime = frameTime - elapsed;
    if (sleepTime > sf::Time::Zero) {
        sf::sleep(sleepTime);  // Sleep to limit the framerate
    }

    clock.restart();
}

void MImGuiWindow::drawGUI(const float deltaTime)
{
    ImGui::SFML::Update(coreWindow, deltaClock.restart());
    ImGuizmo::BeginFrame();

    drawMenuBar();
    showDockSpace();
    drawImGuiSubWindows(deltaTime);

    // Force SFML/GL back to the default framebuffer before ImGui renders
    coreWindow.setActive(true);
    ImGui::SFML::Render(coreWindow);
}

void MImGuiWindow::close() {
    ImGui::SFML::Shutdown();
    MWindow::close();
}

void MImGuiWindow::drawImGuiSubWindows(float deltaTime) {
    for(auto window : MImGuiSubWindowManager::getSubWindows()){
        if(window)
        {
           window->draw(deltaTime);
        }
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
        auto menubarRoot = MMenubarTreeNode::getRoot();
        if (menubarRoot)
            menubarRoot->renderMenubar();
        drawControls();
        ImGui::EndMainMenuBar();
    }
}

void MImGuiWindow::drawControls() {
    MEditorControlsButtons::runtimeControls();
}

void MImGuiWindow::loadFontFile(const SString& pathToFile, float pointSize)
{
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(pathToFile.c_str(), pointSize);
    if (!font)
    {
        MERROR(STR("Unable to load font : ") + pathToFile);
        return;
    }

    if (!ImGui::SFML::UpdateFontTexture())
    {
        MWARN("Unable to load font file");
    }
    ImGui::GetIO().FontDefault = font;
}


