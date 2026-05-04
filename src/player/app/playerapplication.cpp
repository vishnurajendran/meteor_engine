//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "playerapplication.h"

#include "../../core/graphics/core/render-pipeline/render_queue.h"
#include "core/engine/engine_statics.h"
#include "core/engine/gizmos/gizmos.h"

MPlayerApplication::MPlayerApplication() : MApplication(){
    name = STR("MeteorPlayer");
}

void MPlayerApplication::run() {
    if(window == nullptr)
        return;

    startFrame();
    window->clear();
    MSceneManager::getSceneManagerInstance()->update(deltaTime);
    window->update(deltaTime);
    endFrame();
}

void MPlayerApplication::cleanup() {
    MLOG(STR("Cleanup"));
    window->close();
    window = nullptr;
}

void MPlayerApplication::initialise() {
    //appRunning = true;
    MLOG(STR("Initialising Player"));
    window = new MWindow();


    const auto winX = MEngineStatics::getEngineSettings().resX.get();
    const auto winY = MEngineStatics::getEngineSettings().resY.get();
    const auto fps= MEngineStatics::getEngineSettings().fps.get();
    window->initialiseWindow(STR("Meteor Player"), SVector2(winX, winY), fps);

    if(!window->isOpen())
        MERROR(STR("Failed to open window"));

    MSceneManager::registerSceneManager(new MSceneManager());
    MAssetManager::registerAssetManagerInstance(new MAssetManager());
    MAssetManager::getInstance()->refresh();

    pipelineManager.initalise();
    pipelineManager.setRenderTarget(window.get()->getRenderBuffer());

    MLOG(STR("Player Initialised"));
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