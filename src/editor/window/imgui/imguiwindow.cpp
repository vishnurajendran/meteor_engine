//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include <GL/glew.h>
#include "core/utils/meteor_paths.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include "imguiwindow.h"

#include "ImGuizmo.h"
#include "core/engine/gizmos/gizmos.h"
#include "editor/app/shortcut_listener.h"
#include "editor/themes/editor_themes.h"
#include "editor/window/menubar/menubartree.h"
#include "editorcontrolsbuttons.h"
#include "imgui_internal.h"
#include "imguisubwindow.h"
#include "imguisubwindowmanager.h"
#include "imguiwindowconstants.h"

#include "core/application/application.h"
#include "core/engine/domain/engine_domain_subsystem.h"
#include "core/engine/scripting/lua/lua_script_context.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/profiling/profiler.h"
#include "core/utils/logger.h"
#include "editor/profiling/editor_profile_keys.h"

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
    loadFontFile(ENGINE_PATH("meteor_assets/fonts/Open-sans/OpenSansEmoji.ttf"), 18);

    io.FontGlobalScale = 1;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    MEditorThemes::applyTheme();

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
        handleWindowEvents(event);

        // handleWindowEvents may shut down ImGui and close the window.
        // If we keep polling, the next ImGui::SFML::ProcessEvent call
        // hits a destroyed context and corrupts the heap.
        if (!coreWindow.isOpen()) break;
    }

    // The close event handler shuts down ImGui and closes coreWindow.
    // If the window is no longer open, bail out now - calling graphicsFunction()
    // or drawGUI() after ImGui::SFML::Shutdown() would crash or silently fail.
    if (!coreWindow.isOpen())
        return;

    //Call the graphics systems to draw.
    START_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_RENDER)
    if (graphicsFunction)
        graphicsFunction();
    STOP_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_RENDER)

    START_PROFILING_SAMPLE(EditorProfileKeys::EDITOR_IMGUI_DRAW)
    drawGUI(deltaTime);
    STOP_PROFILING_SAMPLE(EditorProfileKeys::EDITOR_IMGUI_DRAW)

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

    // Poll raw input state for the frame, then dispatch shortcuts.
    // Skip shortcut dispatch when ImGui has a text field focused so
    // that typing "S" into a rename box doesn't trigger Ctrl-less bindings.
    SInput::poll();
    if (!ImGui::GetIO().WantTextInput)
        MShortcutListener::poll();

    drawMenuBar();
    MMenubarTreeNode::drawAllPopups();
    showDockSpace();
    drawImGuiSubWindows(deltaTime);
    drawScriptRefreshOverlay();

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
    constexpr auto menuBarOffset = 10.0f;
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarOffset));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - menuBarOffset));
    ImGui::SetNextWindowViewport(viewport->ID);

    // Set window flags for no decoration, background, or inputs
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 20.0f));

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

void MImGuiWindow::drawScriptRefreshOverlay()
{
    static constexpr const char* POPUP_ID = "##MTR_SCRIPT_REFRESH";
    // Only show for refreshes that take a while, so saving a script in a
    // small project doesn't flash a popup. Play is disabled either way.
    static constexpr float SHOW_AFTER_SECONDS = 0.2f;
    static float busyForSeconds = 0.0f;

    auto* domains = MEngineSubsystemRegistry::getSubsystem<MEngineDomainSubsystem>();
    auto* domain  = domains ? domains->getCurrentActiveDomain() : nullptr;

    // During play the runtime domain is on top; the editor domain's refresh
    // only runs while stopped (see MEditorApplication::run).
    const auto* app = MApplication::getAppInstance();
    const bool stopped = app && !app->isSimulating() && !app->isPaused();
    const bool busy = stopped && domain && domain->getScriptContext().isRefreshing();

    busyForSeconds = busy ? busyForSeconds + ImGui::GetIO().DeltaTime : 0.0f;
    const bool show = busy && busyForSeconds >= SHOW_AFTER_SECONDS;

    if (show && !ImGui::IsPopupOpen(POPUP_ID))
        ImGui::OpenPopup(POPUP_ID);

    // Modal: blocks the rest of the editor while it is up.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f));
    if (ImGui::BeginPopupModal(POPUP_ID, nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                               ImGuiWindowFlags_NoSavedSettings))
    {
        if (!show)
        {
            ImGui::CloseCurrentPopup();
        }
        else
        {
            const auto& scripts = domain->getScriptContext();
            const uint32_t done  = scripts.getRefreshDone();
            const uint32_t total = scripts.getRefreshTotal();
            const float fraction = total > 0 ? static_cast<float>(done) / static_cast<float>(total) : 0.0f;

            ImGui::TextUnformatted("Compiling scripts...");
            const SString label = SString::format("{0} / {1}", SString::fromInt(static_cast<int>(done)),
                                                  SString::fromInt(static_cast<int>(total)));
            ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), label.c_str());
        }
        ImGui::EndPopup();
    }
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