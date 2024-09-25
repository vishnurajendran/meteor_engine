//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "playerapplication.h"

MPlayerApplication::MPlayerApplication() : MApplication(){
    name = TEXT("MeteorPlayer");
}

void MPlayerApplication::run() {
    if(window == nullptr)
        return;

    window->clear();
    window->update();
}

void MPlayerApplication::cleanup() {
    MLOG(TEXT("Cleanup"));
    window->close();
    window = nullptr;
}

void MPlayerApplication::initialise() {
    //appRunning = true;
    MLOG(TEXT("Initialising Editor"));
    window = MWindowManager::getSimpleWindow(TEXT("Meteorite Player"));
    if(!window->isOpen())
        MERROR(TEXT("Failed to open window"));
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