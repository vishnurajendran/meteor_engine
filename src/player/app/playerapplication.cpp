//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "playerapplication.h"

#include "../../core/graphics/core/render-pipeline/render_queue.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/audio/impl_miniaudio/audio_engine.h"
#include "core/engine/audio/interfaces/engine_interface.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/domain/engine_domain_subsystem.h"
#include "core/engine/engine_statics.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/input/impl_sfml/sfml_input.h"
#include "core/engine/jobs/job_system.h"
#include "core/engine/animation/animation_subsystem.h"
#include "core/engine/physics/impl/engine/jolt_physics_engine.h"
#include "core/engine/physics/interface/physics_engine_interface.h"
#include "core/engine/scripting/lua/lua_scripting_engine.h"
#include "core/engine/ui/render/ui_render_stage.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"
#include "core/engine/ui/viewport/window_viewport_host.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/scene/scenemanager.h"
#include "core/package/packaged_asset_manager.h"
#include "core/profiling/default_profile_keys.h"
#include "core/profiling/simple_profiler/simple_profiler.h"
#include "core/project/project_descriptor.h"
#include "core/utils/meteor_paths.h"
#include "player/assets/player_asset_manager.h"
#include "core/memory/mnew.h"

void MPlayerApplication::pause(const bool& pause)
{
    playerState = pause ? EPlayerApplicationState::Paused : EPlayerApplicationState::Playing;
}


void MPlayerApplication::registerSubsystems(const std::shared_ptr<MPackagedAssetManager>& package)
{
    // Input - depends on nothing; scripts and shortcuts read it every frame
    MEngineSubsystemRegistry::registerSubsystem<IInputSubsystem, MSfmlInputSubsystem>();

    // Job system — worker threads for MParallel::forEach. Registered early so
    // it is cleaned up (threads joined) after everything that might use it.
    MEngineSubsystemRegistry::registerSubsystem<MJobSystem>();

    // Asset Manager
    // Lazy-loads from the package in published builds; imports everything
    // when run from a project folder.
    // The package goes in straight away: subsystems created after this (the
    // Lua contexts, the shader compiler) read core assets through it.
    auto* assetManager = static_cast<MPlayerAssetManager*>(
        MEngineSubsystemRegistry::registerSubsystem<IAssetManagerSubsystem, MPlayerAssetManager>());
    if (package)
        assetManager->setPackage(package);
    // Render
    pipelineManager = MEngineSubsystemRegistry::registerSubsystem<IRenderPipelineManagerSubsystem, MRenderPipelineManager>(false);
    // Cameras
    MEngineSubsystemRegistry::registerSubsystem<MViewManagement>();
    // Audio
    MEngineSubsystemRegistry::registerSubsystem<IAudioEngineSubsystem, MMiniAudioEngineSubsystem>();

    // Init Physics engine
    physicsEngineRef = MEngineSubsystemRegistry::registerSubsystem<IPhysicsEngineSubsystem, MJoltPhysicsEngine>();

    // Animation - pose evaluation and skinning. Driven by the application
    // between the scene's update and lateUpdate, not ticked.
    MEngineSubsystemRegistry::registerSubsystem<MAnimationSubsystem>();

    // Engine domains - the player's only domain, DOMAIN_0, is the game
    MEngineSubsystemRegistry::registerSubsystem<MEngineDomainSubsystem>();

    // Init Scripting Engine
    MEngineSubsystemRegistry::registerSubsystem<IScriptingEngineSubsystem, MLuaScriptingEngineSubsystem>();

    // Game UI - widgets hold script instances, so after scripting; before the
    // scene manager, so widgets outlive scene loads.
    MEngineSubsystemRegistry::registerSubsystem<MViewportSubsystem>();

    // Scenes - last, since entities use the subsystems above. Registering
    // runs init(), which opens an empty scene.
    sceneManagerRef = MEngineSubsystemRegistry::registerSubsystem<MSceneManager>();
}


MPlayerApplication::MPlayerApplication() : MApplication() {
    name = STR("MeteorPlayer");
}

void MPlayerApplication::run() {
    if(window == nullptr)
        return;

    startFrame();
    window->clear();

    // OS events first: the UI gets input before gameplay, so the
    // SInput.isUIConsuming* flags are current when scripts read them.
    window->pollEvents();
    if (!window->isOpen())
        return;

    // Keyboard state for this frame, before physics and scene updates so
    // scripts see it. (The editor polls from its ImGui window instead.)
    // A scene change a script asked for last frame - before anything ticks.
    if (sceneManagerRef)
        sceneManagerRef->processPendingLoad();

    // Cursor lock first, so the poll below latches the locked position.
    if (uiViewportHost)
        uiViewportHost->update();
    if (auto* input = MEngineSubsystemRegistry::getSubsystem<IInputSubsystem>())
    {
        if (window->hasFocus())
            input->poll();
        else
            input->releaseAll();
    }

    // Paused: no physics, no entity ticks. Animation keeps evaluating (its
    // clocks stop on their own) and the UI keeps running, so a pause menu works.
    const bool paused = isPaused();

    const bool canTickPhysics = physicsEngineRef && physicsEngineRef->canTick();
    // tick physics.
    START_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_PHYSICS)
    if (canTickPhysics && !paused)
        tickPhysics(deltaTime);
    STOP_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_PHYSICS)

    // tick scene manager
    if (sceneManagerRef)
    {
        START_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_SCENE_UPDATE)
        if (!paused)
            sceneManagerRef->update(deltaTime);
        // Poses and skinning: after every onTick (a playClip there shows this
        // frame), before onLateTick (followers see this frame's bones).
        if (auto* animation = MEngineSubsystemRegistry::getSubsystem<MAnimationSubsystem>())
            animation->update(deltaTime);
        // after every onTick, so followers (cameras) see this frame's poses
        if (!paused)
            sceneManagerRef->lateUpdate(deltaTime);
        STOP_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_SCENE_UPDATE)
    }

    // Game UI: onWidgetTick after every onLateTick (a HUD shows this frame's
    // final values), then layout. Drawn by the pipeline's last stage.
    if (auto* ui = MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>())
        ui->update(deltaTime);

    window->update(deltaTime);
}

float MPlayerApplication::getPhysicsStep() const
{
    const auto* settings = MEngineStatics::getEngineSettings();
    const float tickRate  = settings ? settings->physicsTickRate.get() : 60.0f;
    return 1.0f / tickRate;
}

void MPlayerApplication::tickPhysics(float deltaTime)
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

void MPlayerApplication::cleanup() {
    MLOG(STR("[Cleanup] Begin"));

    // Close the scene while every subsystem its entities use is still alive.
    // The scene manager itself is deleted by the subsystem registry.
    if (sceneManagerRef)
        sceneManagerRef->closeActiveScene();
    sceneManagerRef = nullptr;
    MLOG(STR("[Cleanup] Scene closed"));

    MLOG(STR("[Cleanup] Asset manager cleaned"));

    // The UI's viewport outlives the window (the subsystem goes later):
    // detach it from the window before the window goes.
    windowEventSubscription.invalidate();
    if (uiViewport)
        uiViewport->setHost(nullptr);
    uiViewport = nullptr;

    MLOG(STR("[Cleanup] Closing window"));
    if (window != nullptr)
        window->close();
    MLOG(STR("[Cleanup] Window closed"));

    window = nullptr;
    MLOG(STR("[Cleanup] Window released"));

    MEngineStatics::saveAll();
    MLOG(STR("[Cleanup] Complete"));
}

void MPlayerApplication::initialise() {
    MApplication::initialise();

    // A published game has a .manifest (and its paks) next to the executable.
    // Without one, the player runs from the project folder like before.
    auto package = std::make_shared<MPackagedAssetManager>();
    if (!package->loadManifest(SMeteorPaths::engineRoot()))
        package.reset();

    // Settings come from two places:
    //   - project settings (rendering, physics, physics layers): baked into
    //     the manifest in a published game, read-only; read from the project
    //     during development. Never written by the player.
    //   - PlayerSettings.xml: what players may change (resolution, fps,
    //     fullscreen). Next to the executable in a published game.
    MEngineStatics::loadSettings<MEngineSettings>({
        .engineSettingsPath       = SString(),   // defaults; filled in below, never saved
        .physicsLayerSettingsPath = package ? SString() : DEFAULT_PHYSICS_SETTINGS_PATH
    });
    if (package)
    {
        const auto& embedded = package->manifest().getSettings();
        MEngineStatics::loadPhysicsLayersFromXml(embedded.physicsLayersXml);
        MEngineStatics::applyProjectSettingsFromXml(embedded.engineSettingsXml);
    }
    else
    {
        // Development: the project's settings as authored in the editor.
        SString projectSettings;
        if (FileIO::readFile(SString::format("{0}{1}", DEFAULT_SETTINGS_PATH, "EditorSettings.xml"), projectSettings))
            MEngineStatics::applyProjectSettingsFromXml(projectSettings);
    }
    MEngineStatics::loadPlayerSettings(package ? ENGINE_PATH(getEngineSettingsPath()) : getEngineSettingsPath());
    MEngineSubsystemRegistry::init();

    //appRunning = true;
    MVERBOSE(STR("Initialising Player"));
    window = mnew<MWindow>();


    const auto winX = MEngineStatics::getEngineSettings()->resX.get();
    const auto winY = MEngineStatics::getEngineSettings()->resY.get();
    const auto fps= MEngineStatics::getEngineSettings()->fps.get();
    const SString title = package && !package->manifest().getGameInfo().name.empty()
                              ? package->manifest().getGameInfo().name
                              : STR("Meteor Player");
    window->setFullscreen(MEngineStatics::getPlayerSettings()->fullscreen.get());
    window->initialiseWindow(title, SVector2(winX, winY), fps);

    if(!window->isOpen())
        MERROR(STR("Failed to open window"));

    registerSubsystems(package);

    MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->refresh();

    pipelineManager->init();
    pipelineManager->setRenderTarget(window->getRenderBuffer());
    pipelineManager->getPipeline()->addStage<MUIRenderStage>();   // game UI, last
    createUIViewport();
    subscribeMouseInput();

    loadStartupScene(package.get());

    MVERBOSE(STR("Player Initialised"));
}

void MPlayerApplication::createUIViewport()
{
    auto* ui = MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>();
    if (!ui || !window)
        return;

    uiViewportHost = std::make_unique<MWindowViewportHost>(window.get());
    uiViewport     = ui->createViewport(uiViewportHost.get());

    // Window events reach the UI as they are polled (start of the frame).
    windowEventSubscription = window->onEvent.add([this](const sf::Event& event)
    {
        if (uiViewport)
            uiViewport->processWindowEvent(event);
    });
}

void MPlayerApplication::subscribeMouseInput()
{
    if (!window)
        return;

    // The game viewport is the whole window, so window pixels are viewport
    // pixels. Latched by input->poll() in run(), after the events.
    mouseInputSubscription = window->onEvent.add([](const sf::Event& event)
    {
        auto* input = MEngineSubsystemRegistry::getSubsystem<IInputSubsystem>();
        if (!input)
            return;

        auto toViewport = [](sf::Vector2i p) { return SVector2(static_cast<float>(p.x), static_cast<float>(p.y)); };

        if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
            input->onMouseMoved(toViewport(moved->position));
        else if (const auto* raw = event.getIf<sf::Event::MouseMovedRaw>())
            input->onMouseRawMoved(SVector2(static_cast<float>(raw->delta.x), static_cast<float>(raw->delta.y)));
        else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
        {
            input->onMouseMoved(toViewport(pressed->position));
            input->onMouseButton(MSfmlInputSubsystem::toMouseButtonId(pressed->button), true);
        }
        else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
        {
            input->onMouseMoved(toViewport(released->position));
            input->onMouseButton(MSfmlInputSubsystem::toMouseButtonId(released->button), false);
        }
        else if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
        {
            if (wheel->wheel == sf::Mouse::Wheel::Vertical)
                input->onMouseScrolled(wheel->delta);
        }
        else if (event.is<sf::Event::MouseLeft>())
            input->onMouseLeft();
        // Focus loss: run() calls releaseAll() while the window is unfocused.
    });
}

void MPlayerApplication::loadStartupScene(const MPackagedAssetManager* package)
{
    SString scenePath;
    if (package)
    {
        // Prefer the GUID, so a scene moved after publishing still resolves.
        const auto& startup = package->manifest().getStartupScene();
        const auto* entry   = package->manifest().findByGuid(startup.guid);
        scenePath = entry ? entry->path : startup.path;
    }
    else if (SMeteorPaths::hasProjectFile())
    {
        SProjectDescriptor descriptor;
        if (descriptor.load(SMeteorPaths::projectFile()))
            scenePath = descriptor.startupScene;
    }

    if (scenePath.empty())
    {
        MWARN(STR("MPlayerApplication:: no startup scene set"));
        return;
    }

    MLOG(STR("MPlayerApplication:: loading startup scene ") + scenePath);
    if (sceneManagerRef)
        sceneManagerRef->loadSceneImmediate(scenePath);
}

bool MPlayerApplication::isRunning() const {
    if(window == nullptr)
        return false;

    return window->isOpen();
}

#if PLAYER_APPLICATION
MApplication* getAppInstance(){
    MLOG("Creating Player Application instance");
    return mnew<MPlayerApplication>();
}
#endif
