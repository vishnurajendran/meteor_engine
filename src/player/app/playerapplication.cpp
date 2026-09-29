//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "playerapplication.h"

#include "../../core/graphics/core/render-pipeline/render_queue.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/audio/impl_miniaudio/audio_engine.h"
#include "core/engine/audio/interfaces/engine_interface.h"
#include "core/engine/engine_statics.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/physics/impl/engine/jolt_physics_engine.h"
#include "core/engine/physics/interface/physics_engine_interface.h"
#include "core/engine/scripting/lua/lua_scripting_engine.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/profiling/default_profile_keys.h"
#include "core/profiling/simple_profiler/simple_profiler.h"

void MPlayerApplication::pause(const bool& pause)
{
    playerState = pause ? EPlayerApplicationState::Paused : EPlayerApplicationState::Playing;
}


void MPlayerApplication::registerSubsystems()
{
    // Asset Manager
    MEngineSubsystemRegistry::registerSubsystem<IAssetManagerSubsystem, MAssetManager>();
    // Render
    pipelineManager = MEngineSubsystemRegistry::registerSubsystem<IRenderPipelineManagerSubsystem, MRenderPipelineManager>(false);
    // Audio
    MEngineSubsystemRegistry::registerSubsystem<IAudioEngineSubsystem, MMiniAudioEngineSubsystem>();

    // Init Physics engine
    physicsEngineRef = MEngineSubsystemRegistry::registerSubsystem<IPhysicsEngineSubsystem, MJoltPhysicsEngine>();

    // Init Scripting Engine
    MEngineSubsystemRegistry::registerSubsystem<IScriptingEngineSubsystem, MLuaScriptingEngineSubsystem>();
}


MPlayerApplication::MPlayerApplication() : MApplication() {
    name = STR("MeteorPlayer");
}

void MPlayerApplication::run() {
    if(window == nullptr)
        return;

    startFrame();
    window->clear();

    const auto fixedStep = getPhysicsStep();
    const bool canTickPhysics = physicsEngineRef && physicsEngineRef->canTick();
    // tick physics.
    START_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_PHYSICS)
    if (canTickPhysics)
        tickPhysics(deltaTime);
    STOP_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_PHYSICS)

    // tick scene manager
    if (sceneManagerRef)
    {
        START_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_SCENE_UPDATE)
        sceneManagerRef->update(deltaTime);
        STOP_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_SCENE_UPDATE)

        if (internal_tickSpatialFixedUpdateFlag)
        {
            internal_tickSpatialFixedUpdateFlag = false;
            START_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_SCENE_FIXED_UPDATE)
            sceneManagerRef->fixedUpdate(fixedStep);
            STOP_PROFILING_SAMPLE(DefaultProfileKeys::APPLICATION_SCENE_FIXED_UPDATE)
        }
    }

    window->update(deltaTime);
    endFrame();
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

    while (physicsAccumulator >= fixedStep)
    {
        physicsEngineRef->tick(fixedStep);
        physicsAccumulator -= fixedStep;
        internal_tickSpatialFixedUpdateFlag = true;
    }
}

void MPlayerApplication::cleanup() {
    MLOG(STR("[Cleanup] Begin"));

    if (sceneManagerRef)
        sceneManagerRef->closeActiveScene();

    MLOG(STR("[Cleanup] Deleting SceneManager"));
    delete sceneManagerRef;
    sceneManagerRef = nullptr;
    MLOG(STR("[Cleanup] SceneManager deleted"));

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

void MPlayerApplication::initialise() {
    MApplication::initialise();

    // init engine settings.
    MEngineStatics::loadSettings<MEngineSettings>({
        .engineSettingsPath = getEngineSettingsPath(),
        .physicsLayerSettingsPath = DEFAULT_PHYSICS_SETTINGS_PATH
    });
    MEngineSubsystemRegistry::init();

    //appRunning = true;
    MVERBOSE(STR("Initialising Player"));
    window = new MWindow();


    const auto winX = MEngineStatics::getEngineSettings()->resX.get();
    const auto winY = MEngineStatics::getEngineSettings()->resY.get();
    const auto fps= MEngineStatics::getEngineSettings()->fps.get();
    window->initialiseWindow(STR("Meteor Player"), SVector2(winX, winY), fps);

    if(!window->isOpen())
        MERROR(STR("Failed to open window"));

    registerSubsystems();

    sceneManagerRef = new MSceneManager();
    MSceneManager::registerSceneManager(sceneManagerRef);

    auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    assetManager->refresh();

    pipelineManager->init();
    pipelineManager->setRenderTarget(window->getRenderBuffer());

    MVERBOSE(STR("Player Initialised"));
}

bool MPlayerApplication::isRunning() const {
    if(window == nullptr)
        return false;

    return window->isOpen();
}

#if PLAYER_APPLICATION
MApplication* getAppInstance(){
    MLOG("Creating Player Application instance");
    return new MPlayerApplication();
}
#endif