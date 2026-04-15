//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "playerapplication.h"

#include "../../core/graphics/core/render-pipeline/render_queue.h"
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

    // runt the render pipeline
    pipelineManager.preRender();
    pipelineManager.render();
    pipelineManager.postRender();

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
    window->initialiseWindow(STR("Meteor Player"), MWindow::DEFAULT_WINDOW_SIZE, MWindow::DEFAULT_FPS);
    if(!window->isOpen())
        MERROR(STR("Failed to open window"));

    MSceneManager::registerSceneManager(new MSceneManager());
    MAssetManager::registerAssetManagerInstance(new MAssetManager());
    MAssetManager::getInstance()->refresh();
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