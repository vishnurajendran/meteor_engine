//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorapplication.h"
#include "core/utils/meteor_paths.h"

#include <windows.h>

#include "core/default_settings_paths.h"
#include "core/engine/composition/composition_utility.h"
#include "core/engine/audio/impl_miniaudio/audio_engine.h"
#include "core/engine/domain/engine_domain_subsystem.h"
#include "core/engine/scripting/lua/lua_script_context.h"
#include "core/engine/engine_statics.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/physics/impl/engine/jolt_physics_engine.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/render_queue.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/editorrenderstages/gizmos/gizmo_stage.h"
#include "editor/editorscenemanager/editorscenemanager.h"
#include "editor/editorwindows/assetwindow/editorassetwindow.h"
#include "editor/helper/scene_io.h"
#include "editor/settings/editor_settings.h"
#include "editor/window/imgui/imguiwindow.h"
#include "editor/window/menubar/menubartree.h"
#include "simulation_manager.h"

#include "core/profiling/profiler.h"
#include "core/engine/scripting/lua/lua_scripting_engine.h"
#include "core/engine/scripting/interface/scripting_engine_interface.h"
#include "core/utils/guid.h"
#include "editor/editorwindows/console/editorconsolewindow.h"
#include "editor/editorwindows/hierarchy/editorhierarchywindow.h"
#include "editor/editorwindows/inspector/editorinspectorwindow.h"
#include "editor/editorwindows/scene/editorsceneviewwindow.h"
#include "editor/publishing/game_publisher.h"

MObject* MEditorApplication::SelectedObject = nullptr;

MEditorApplication::MEditorApplication() : MApplication(){
    name = STR("MeteoriteEditor");
}


void MEditorApplication::run() {
    if(window == nullptr)
        return;

    START_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_RUN)

    startFrame();
    window->clear();

    const bool canTickPhysics = physicsEngineRef && physicsEngineRef->canTick();
    const bool canSimulate = simulationState == EEditorSimulationState::Simulating;

    // tick physics.
    if (canSimulate)
    {
        START_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_PHYSICS)
        if (simulationState && canTickPhysics)
           tickPhysics(deltaTime);
        STOP_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_PHYSICS)
    }

    // tick scene manager
    if (sceneManagerRef)
    {
        START_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_SCENE_UPDATE)
        sceneManagerRef->update(deltaTime);
        // after every onTick, so followers (cameras) see this frame's poses
        sceneManagerRef->lateUpdate(deltaTime);
        STOP_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_SCENE_UPDATE)
    }

    // While a publish runs, nothing may change the project: file watcher
    // events wait (they are handled once it finishes), and so do script
    // index syncing and script recompiles below.
    const bool publishing = MGamePublisher::isPublishing();

    // tick asset manager
    if (assetManagerRef && !publishing)
        assetManagerRef->tickHotReload();

    // keep VS Code's import() index in step with the project's scripts
    if (!publishing)
        projectManager.syncScriptIndex();

    // Script refresh: whenever a script changes, the editor domain recompiles
    // every script (a frame budget at a time) to report errors before Play.
    // Only while stopped - during play the runtime domain is on top, and the
    // refresh catches up once Stop removes it.
    if (simulationState == EEditorSimulationState::Stopped && !publishing)
    {
        auto* domains = MEngineSubsystemRegistry::getSubsystem<MEngineDomainSubsystem>();
        if (auto* domain = domains ? domains->getCurrentActiveDomain() : nullptr)
            domain->getScriptContext().refreshStep(SCRIPT_REFRESH_BUDGET_MS);
    }

    window->update(deltaTime);

    endFrame();

    STOP_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_RUN)
}

void MEditorApplication::cleanup() {
    MLOG(STR("[Cleanup] Begin"));

    // Close the scene while every subsystem its entities use is still alive.
    // The scene manager itself is deleted by the subsystem registry.
    if (sceneManagerRef)
        sceneManagerRef->closeActiveScene();
    sceneManagerRef = nullptr;
    MLOG(STR("[Cleanup] Scene closed"));

    MLOG(STR("[Cleanup] Cleaning render pipeline"));
    if (pipelineManager)
        pipelineManager->cleanup();
    MLOG(STR("[Cleanup] Render pipeline cleaned"));

    MLOG(STR("[Cleanup] Cleaning asset manager"));
    if (assetManagerRef)
        assetManagerRef->cleanup();
    MLOG(STR("[Cleanup] Asset manager cleaned"));

    MLOG(STR("[Cleanup] Closing window"));
    if (window != nullptr)
        window->close();
    MLOG(STR("[Cleanup] Window closed"));

    window = nullptr;
    MLOG(STR("[Cleanup] Window released"));

    MEngineStatics::saveAll();
    MLOG(STR("[Cleanup] Complete"));
}

void MEditorApplication::registerSubsystems()
{
    // Asset Manager
    assetManagerRef = dynamic_cast<MEditorAssetManager*>(
        MEngineSubsystemRegistry::registerSubsystem<IAssetManagerSubsystem, MEditorAssetManager>());

    // Render pipeline, does not autoInit - we need to delay this.
    pipelineManager =
        MEngineSubsystemRegistry::registerSubsystem<IRenderPipelineManagerSubsystem, MRenderPipelineManager>(false);

    // Audio System
    audioEngineRef = MEngineSubsystemRegistry::registerSubsystem<IAudioEngineSubsystem, MMiniAudioEngineSubsystem>();

    // Init Physics engine
    physicsEngineRef = MEngineSubsystemRegistry::registerSubsystem<IPhysicsEngineSubsystem, MJoltPhysicsEngine>();

    // Engine domains - creates DOMAIN_0, the editor's script sandbox
    MEngineSubsystemRegistry::registerSubsystem<MEngineDomainSubsystem>();

    // Init Scripting Engine
    MEngineSubsystemRegistry::registerSubsystem<IScriptingEngineSubsystem, MLuaScriptingEngineSubsystem>();

    // Editor Simulation Manager, we will register as the concrete class for now. in the future, if
    // we need to explose APIs for this system, then wire that through an interface.
    simulationManagerSubsystem = MEngineSubsystemRegistry::registerSubsystem<MEditorSimulationManagerSubsystem, MEditorSimulationManagerSubsystem>(false);

}

void MEditorApplication::notifySimulationStateChange()
{
    onSimulationStateChanged.broadcast(simulationState);
}

void MEditorApplication::tickPhysics(float deltaTime)
{
    const auto fixedStep = getPhysicsStep();
    physicsAccumulator += deltaTime;
    // Cap accumulation to avoid a spiral-of-death after a hitch.
    constexpr float maxAccumulation = 0.2f;
    if (physicsAccumulator > maxAccumulation)
        physicsAccumulator = maxAccumulation;

    // Each physics step runs onFixedTick first (so forces land on that step),
    // then the step, then lets body entities record the pose it produced.
    while (physicsAccumulator >= fixedStep)
    {
        if (sceneManagerRef)
        {
            // records the last step's cost when several run in one frame
            START_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_SCENE_FIXED_UPDATE)
            sceneManagerRef->fixedUpdate(fixedStep);
            STOP_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_SCENE_FIXED_UPDATE)
        }

        physicsEngineRef->tick(fixedStep);
        physicsAccumulator -= fixedStep;

        if (sceneManagerRef)
            sceneManagerRef->postPhysicsStep(fixedStep);
    }

    // The leftover time says how far this frame is between the last two
    // steps. Body entities blend their last two poses by it, before any
    // onTick reads them.
    if (sceneManagerRef)
        sceneManagerRef->interpolatePhysics(physicsAccumulator / fixedStep);
}

void MEditorApplication::initialise() {

    MApplication::initialise();

    // Read the .mtproj and make sure the project folders exist before any
    // settings are loaded or saved (they live under .engine_data/settings).
    projectManager.prepare();

    // init engine settings.
    MEngineStatics::loadSettings<MEditorSettings>(SSettingsPaths
    {
        .engineSettingsPath = getEngineSettingsPath(),
        .physicsLayerSettingsPath = DEFAULT_PHYSICS_SETTINGS_PATH
    });

    registerSubsystems();

    std::thread splashThread(&MEditorApplication::showSplashScreen, this);

    //appRunning = true;
    MVERBOSE(STR("Initialising Editor"));
    window = new MImGuiWindow();
    const auto winX = MEngineStatics::getEngineSettings()->resX.get();
    const auto winY = MEngineStatics::getEngineSettings()->resY.get();
    const auto fps= MEngineStatics::getEngineSettings()->fps.get();
    window->initialiseWindow(STR("Meteorite Editor - ") + projectManager.getDescriptor().name, SVector2(winX, winY), fps);
    windowResizedSubscription = window->onResized.add([](const SVector2& size)
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
    // Last opened scene (stored per project), else the project's startup scene.
    SString startScene;
    if (const auto settings = dynamic_cast<MEditorSettings*>(MEngineStatics::getEngineSettings()))
        startScene = SString(settings->lastOpenedScene.get());
    if (startScene.empty())
        startScene = projectManager.getDescriptor().startupScene;
    if (startScene.empty())
        return;

    MVERBOSE(SString::format("[MEditorApplication]::Loading scene {0}", startScene));
    sceneManagerRef->loadScene(startScene);

}

bool MEditorApplication::isRunning() const
{
    if (window == nullptr)
        return false;

    return window->isOpen();
}

float MEditorApplication::getPhysicsStep() const
{
    const auto* settings = MEngineStatics::getEngineSettings();
    const float tickRate  = settings ? settings->physicsTickRate.get() : 60.0f;
    return 1.0f / tickRate;
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
    splashTexture.loadFromFile(ENGINE_PATH("meteor_assets/splash.png").str());

    sf::Sprite sprite(splashTexture);
    sprite.setScale( { splashWindow.getSize().x / (float)splashTexture.getSize().x,
                    splashWindow.getSize().y / (float)splashTexture.getSize().y});

    sf::Font font;
    font.openFromFile(ENGINE_PATH("meteor_assets/fonts/open-sans/OpenSans-Regular.ttf").str());
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
    // Project folder + engine install (meteor_assets/), project templates,
    // __PROJECT_NAME__. Must happen before the first refresh.
    projectManager.applyToAssetManager(*assetManagerRef);
    assetManagerRef->refresh();
    pipelineManager->init(); // manual init
    pipelineManager->getPipeline()->addStage<MGizmoStage>();

    // setup scene manager
    // Registering runs init(), which opens an empty scene.
    sceneManagerRef = static_cast<MEditorSceneManager*>(
        MEngineSubsystemRegistry::registerSubsystem<MSceneManager, MEditorSceneManager>());
    sceneLoadedSubscription = sceneManagerRef->onSceneLoaded.add([](MScene*) { SelectedObject = nullptr; });

    // Resetting a composition instance replaces its entities - move the
    // selection to the new root if it pointed anywhere inside the old one,
    // otherwise it would dangle once the scene frees the old entities.
    // (Was assigned in every inspector drawer's constructor; subscribing
    // there would now add a duplicate listener per drawer.)
    instanceReplacedSubscription = MCompositionUtility::onInstanceReplaced.add(
        [](MSpatialEntity* oldRoot, MSpatialEntity* newRoot)
        {
            auto* selected = dynamic_cast<MSpatialEntity*>(SelectedObject);
            for (MSpatialEntity* e = selected; e; e = e->getParent())
            {
                if (e == oldRoot)
                {
                    SelectedObject = newRoot;
                    return;
                }
            }
        });

    if (simulationManagerSubsystem)
        simulationManagerSubsystem->init();
}

bool MEditorApplication::canStartSimulation(SString& outReason) const
{
    if (MGamePublisher::isPublishing())
    {
        outReason = "A publish is in progress";
        return false;
    }

    auto* domains = MEngineSubsystemRegistry::getSubsystem<MEngineDomainSubsystem>();
    auto* domain  = domains ? domains->getCurrentActiveDomain() : nullptr;
    if (!domain)
        return true;

    // Before Play the editor domain is on top.
    const auto& scripts = domain->getScriptContext();
    if (scripts.isRefreshing())
    {
        outReason = "Scripts are compiling";
        return false;
    }
    if (scripts.getErrorCount() > 0)
    {
        outReason = SString::format("{0} script(s) failed to compile - see the console",
                                    SString::fromInt(static_cast<int>(scripts.getErrorCount())));
        return false;
    }
    return true;
}

void MEditorApplication::startSimulation()
{
    if (isSimulating())
        return;

    // Every way into Play goes through here, not just the button.
    SString reason;
    if (!canStartSimulation(reason))
    {
        MWARN(SString("Cannot start play mode: ") + reason);
        return;
    }

    simulationState = EEditorSimulationState::Simulating;
    notifySimulationStateChange();
}

void MEditorApplication::stopSimulation()
{
    if (simulationState == EEditorSimulationState::Stopped)
        return;
    simulationState = EEditorSimulationState::Stopped;
    notifySimulationStateChange();
}

void MEditorApplication::pauseSimulation()
{
    if (isPaused())
        return;
    simulationState = EEditorSimulationState::SimulationPaused;
    notifySimulationStateChange();
}

void MEditorApplication::exit()
{
    MEngineStatics::saveAll();
    auto editorApp = dynamic_cast<MEditorApplication*>(appInst);
    if (editorApp != nullptr)
        editorApp->window->close();
}

MCameraEntity* MEditorApplication::getSceneCamera()
{
    auto editorApp = dynamic_cast<MEditorApplication*>(appInst);
    if (editorApp == nullptr)
        return nullptr;

    return editorApp->sceneManagerRef->getEditorSceneCamera();
}



#if EDITOR_APPLICATION
MApplication* getAppInstance(){
    MVERBOSE("Creating Editor Application instance");
    return new MEditorApplication();
}
#endif