//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorapplication.h"

#include "../../core/graphics/core/render-pipeline/render_queue.h"
#include "core/engine/engine_statics.h"
#include "core/engine/gizmos/gizmos.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/editorrenderstages/gizmos/gizmo_stage.h"
#include "editor/editorscenemanager/editorscenemanager.h"
#include "editor/editorwindows/assetwindow/editorassetwindow.h"
#include "editor/helper/scene_io.h"
#include "editor/settings/editor_settings.h"
#include "editor/window/imgui/imguiwindow.h"
#include "editor/window/menubar/menubartree.h"


MObject* MEditorApplication::SelectedObject = nullptr;
MEditorApplication* MEditorApplication::editorInst = nullptr;


MEditorApplication::MEditorApplication() : MApplication(){
    name = STR("MeteoriteEditor");
}


void MEditorApplication::run() {
    if(window == nullptr)
        return;

    startFrame();
    window->clear();

    if (sceneManagerRef)
        sceneManagerRef->update(deltaTime);

    if (assetManagerRef)
        assetManagerRef->tickHotReload();

    window->update(deltaTime);

    // Process global keyboard shortcuts after ImGui has processed input.
    processGlobalShortcuts();

    endFrame();
}

void MEditorApplication::processGlobalShortcuts()
{
    // Only process when no text input is active — avoids capturing
    // Ctrl+S while the user is typing in a text field.
    if (ImGui::GetIO().WantTextInput) return;

    const bool ctrl  = ImGui::GetIO().KeyCtrl;
    const bool shift = ImGui::GetIO().KeyShift;

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
    {
        if (shift)
            saveAllDirty();
        else
            saveSelected();
    }
}

void MEditorApplication::saveSelected()
{
    // If an asset is selected, save it.
    if (auto* asset = dynamic_cast<MAsset*>(SelectedObject))
    {
        if (asset->save())
        {
            asset->clearDirty();
            MLOG(SString::format("Saved asset: {0}", asset->getPath()));
        }
        return;
    }

    // Otherwise, save the current scene.
    if (!sceneManagerRef) return;

    SString path = sceneManagerRef->getActiveScenePath();
    if (path.empty())
        path = MSceneIO::getCurrentPath();

    if (!path.empty())
    {
        sceneManagerRef->saveCurrentScene(path);
        auto* scene = sceneManagerRef->getActiveScene();
        if (scene) scene->clearDirty();
        MLOG(SString::format("Saved scene: {0}", path));
    }
    else
    {
        MWARN("No scene path known — use File > Save Scene As");
    }
}

void MEditorApplication::saveAllDirty()
{
    // Save all dirty assets.
    int count = MAssetManager::getInstance()->saveDirtyAssets();

    // Save scene if dirty.
    if (sceneManagerRef)
    {
        auto* scene = sceneManagerRef->getActiveScene();
        if (scene && scene->isDirty())
        {
            SString path = sceneManagerRef->getActiveScenePath();
            if (path.empty()) path = MSceneIO::getCurrentPath();

            if (!path.empty())
            {
                sceneManagerRef->saveCurrentScene(path);
                scene->clearDirty();
                ++count;
            }
        }
    }

    MLOG(SString::format("Save All: {0} items saved", count));
}


void MEditorApplication::cleanup() {
    MLOG(STR("Cleanup..."));

    MLOG(STR("Deleting SceneManager"));
    delete sceneManagerRef;

    MLOG(STR("Closing Window"));
    if (window != nullptr)
        window->close();
    window = nullptr;
    MLOG(STR("Editor Application Cleanup Complete"));
}

void MEditorApplication::initialise() {

    // init engine settings.
    MEngineStatics::loadEngineSettings<MEditorSettings>(getEngineSettingsPath());

    std::thread splashThread(&MEditorApplication::showSplashScreen, this);
    editorInst = this;
    //appRunning = true;
    MLOG(STR("Initialising Editor"));
    window = new MImGuiWindow();
    const auto winX = MEngineStatics::getEngineSettings()->resX.get();
    const auto winY = MEngineStatics::getEngineSettings()->resY.get();
    const auto fps= MEngineStatics::getEngineSettings()->fps.get();
    window->initialiseWindow(STR("Meteorite Editor"), SVector2(winX, winY), fps);
    window->setWindowResizeCallback([this](const SVector2& size)
    {
        MLOG(STR("Meteorite:: Resized Window"));
        MEngineStatics::getEngineSettings()->resX.set(size.x);
        MEngineStatics::getEngineSettings()->resY.set(size.y);
        MEngineStatics::saveAll();
    });

    // add a gizmo pipelin
    pipelineManager.getPipeline().addStage<MGizmoStage>();

    // call intialise before use
    window->setGraphicsCall([this]()
    {
        pipelineManager.preRender();
        pipelineManager.render();
        pipelineManager.postRender();
    });

    if(window == nullptr){
        MERROR(STR("Invalid Window Type"));
        return;
    }
    if(!window->isOpen()) {
        MERROR(STR("Failed to open window"));
        return;
    }
    window->setVisible(false);
    loadPrerequisites();

    // Load first, so that it can log everything from here on out.
    subWindows.push_back(new MEditorConsoleWindow());
    subWindows.push_back(new MEditorHierarchyWindow());
    subWindows.push_back(new MEditorInspectorWindow());
    subWindows.push_back(new MEditorSceneViewWindow());
    subWindows.push_back(new MEditorAssetWindow());
    MLOG(STR("Loaded Editor Windows"));

    MMenubarTreeNode::buildTree();
    MLOG(STR("Built Menubar Items"));

    MGizmos::enableGizmos(true);

    window->setVisible(true);
    splashShowing = false;
    splashThread.join();
}

bool MEditorApplication::isRunning() const
{
    if (window == nullptr)
        return false;

    return window->isOpen();
}

void MEditorApplication::showSplashScreen()
{
    sf::RenderWindow splashWindow(sf::VideoMode(sf::Vector2u(960, 540)), "Meteorite-Splash", sf::Style::None);
    splashWindow.setFramerateLimit(60);

    splashWindow.setActive(true);
    splashWindow.setVisible(true);
    splashWindow.requestFocus();

    //Hack to force to top
    HWND hwnd = splashWindow.getNativeHandle();  // SFML lets you grab the native Win32 handle
    ShowWindow(hwnd, SW_SHOW);               // Ensure it is shown
    SetForegroundWindow(hwnd);               // Bring to foreground
    SetFocus(hwnd);


    sf::Texture splashTexture;
    splashTexture.loadFromFile("meteor_assets/splash.png");

    sf::Sprite sprite(splashTexture);
    sprite.setScale( { splashWindow.getSize().x / (float)splashTexture.getSize().x,
                    splashWindow.getSize().y / (float)splashTexture.getSize().y});

    sf::Font font;
    font.openFromFile("meteor_assets/fonts/open-sans/OpenSans-Regular.ttf");
    sf::Text text(font, "Loading Meteorite...");
    text.setCharacterSize(12); // in pixels
    text.setFillColor(sf::Color::White);
    text.setPosition(sf::Vector2f(30, 500));

    while (splashShowing && splashWindow.isOpen())
    {
        std::optional<sf::Event> event;
        while (event = splashWindow.pollEvent())
        {
            continue;
        }

        text.setString(MLogger::getLastMessage().c_str());
        splashWindow.clear();
        splashWindow.draw(sprite);
        splashWindow.draw(text);
        splashWindow.display();
    }


    splashWindow.close();
}

void MEditorApplication::loadPrerequisites()
{
    //Refresh Asset Manager
    assetManagerRef = new MEditorAssetManager();
    MAssetManager::registerAssetManagerInstance(assetManagerRef);
    MAssetManager::getInstance()->refresh();

    //setup scene manager
    sceneManagerRef = new MEditorSceneManager();
    MSceneManager::registerSceneManager(sceneManagerRef);
    sceneManagerRef->registerOnLoadCallback([this](MScene* scene)
    {
        SelectedObject = nullptr;
    });

    //load render pipeline
    pipelineManager.initalise();
}

void MEditorApplication::exit()
{
    if (editorInst != nullptr)
        editorInst->window->close();
}

MCameraEntity* MEditorApplication::getSceneCamera()
{
    if (editorInst == nullptr)
        return nullptr;

    return editorInst->sceneManagerRef->getEditorSceneCamera();
}

#if EDITOR_APPLICATION
MApplication* getAppInstance(){
    MLOG("Creating Editor Application instance");
    return new MEditorApplication();
}
#endif