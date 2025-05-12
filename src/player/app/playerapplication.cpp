//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "playerapplication.h"

#include "core/graphics/core/render_queue.h"

MPlayerApplication::MPlayerApplication() : MApplication(){
    name = STR("MeteorPlayer");
}

void MPlayerApplication::run() {
    if(window == nullptr)
        return;
    window->clear();
    MSceneManager::getSceneManagerInstance()->update(0.0f);
    MRenderQueue::requestDrawCalls();
    window->update();
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