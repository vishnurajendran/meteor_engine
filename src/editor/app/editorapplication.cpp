//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorapplication.h"

#include "../../core/graphics/core/render-pipeline/render_queue.h"
#include "core/engine/audio/impl_miniaudio/audio_engine.h"
#include "core/engine/engine_statics.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/subsystem/subsystem_registry.h"
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
    endFrame();
}

void MEditorApplication::cleanup() {
    MVERBOSE(STR("Cleanup..."));

    MVERBOSE(STR("Deleting SceneManager"));
    delete sceneManagerRef;

    MVERBOSE(STR("Closing Window"));
    if (window != nullptr)
        window->close();
    window = nullptr;
    MVERBOSE(STR("Editor Application Cleanup Complete"));
}

void MEditorApplication::addSubsystems()
{
    // Asset Manager
    assetManagerRef = dynamic_cast<MEditorAssetManager*>(MEngineSubsystemRegistry::registerSubsystem<IAssetManagerSubsystem, MEditorAssetManager>());
    // Render pipeline, does not autoInit - we need to delay this.
    pipelineManager = MEngineSubsystemRegistry::registerSubsystem<IRenderPipelineManagerSubsystem, MRenderPipelineManager>(false);
    // Audio System
    MEngineSubsystemRegistry::registerSubsystem<IAudioEngineSubsystem, MMiniAudioEngineSubsystem>();
}

void MEditorApplication::initialise() {

    // init engine settings.
    addSubsystems();
    MEngineStatics::loadEngineSettings<MEditorSettings>(getEngineSettingsPath());

    std::thread splashThread(&MEditorApplication::showSplashScreen, this);
    editorInst = this;
    //appRunning = true;
    MVERBOSE(STR("Initialising Editor"));
    window = new MImGuiWindow();
    const auto winX = MEngineStatics::getEngineSettings()->resX.get();
    const auto winY = MEngineStatics::getEngineSettings()->resY.get();
    const auto fps= MEngineStatics::getEngineSettings()->fps.get();
    window->initialiseWindow(STR("Meteorite Editor"), SVector2(winX, winY), fps);
    window->setWindowResizeCallback([this](const SVector2& size)
    {
        MVERBOSE(STR("Meteorite:: Resized Window"));
        MEngineStatics::getEngineSettings()->resX.set(size.x);
        MEngineStatics::getEngineSettings()->resY.set(size.y);
        MEngineStatics::saveAll();
    });

    // call intialise before use
    window->setGraphicsCall([this]()
    {
        if (pipelineManager == nullptr) return;

        pipelineManager->preRender();
        pipelineManager->render();
        pipelineManager->postRender();
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
    MVERBOSE(STR("Loaded Editor Windows"));

    MMenubarTreeNode::buildTree();
    MVERBOSE(STR("Built Menubar Items"));

    MGizmos::enableGizmos(true);

    window->setVisible(true);
    splashShowing = false;
    splashThread.join();


    // post-load
    if (const auto settings = dynamic_cast<MEditorSettings*>(MEngineStatics::getEngineSettings())){
        const auto path = SString(settings->lastOpenedScene.get());
        if (path.empty())
            return;
        MVERBOSE(SString::format("[MEditorApplication]::Loading last opened scene {0}", path));
        sceneManagerRef->loadScene(path);
    }

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
    assetManagerRef->refresh();
    pipelineManager->init(); // manual init
    pipelineManager->getPipeline()->addStage<MGizmoStage>();

    //setup scene manager
    sceneManagerRef = new MEditorSceneManager();
    MSceneManager::registerSceneManager(sceneManagerRef);
    sceneManagerRef->registerOnLoadCallback([this](MScene* scene)
    {
        SelectedObject = nullptr;
    });
}


void MEditorApplication::exit()
{
    MEngineStatics::saveAll();
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
    MVERBOSE("Creating Editor Application instance");
    return new MEditorApplication();
}
#endif