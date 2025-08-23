//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "playerapplication.h"

#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/render_queue.h"

MPlayerApplication::MPlayerApplication() : MApplication(){
    name = STR("MeteorPlayer");
}

void MPlayerApplication::run() {
    if(window == nullptr)
        return;

    startFrame();
    window->clear();
    MSceneManager::getSceneManagerInstance()->update(deltaTime);
    MRenderQueue::requestDrawCalls();
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
    window = new MWindow(STR("Meteor Player"));
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