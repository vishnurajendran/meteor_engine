//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "playerapplication.h"

MPlayerApplication::MPlayerApplication() : MApplication(){
    name = TEXT("MeteorPlayer");
}

void MPlayerApplication::run() {
    MApplication::run();
    MLOG(TEXT("Run"));
}

void MPlayerApplication::cleanup() {
    MApplication::cleanup();
    MLOG(TEXT("Cleanup"));
}

void MPlayerApplication::initialise() {
    MApplication::initialise();
    //appRunning = true;
    MLOG(TEXT("Initialise"));
}

#if PLAYER_APPLICATION
MApplication* getAppInstance(){
    MLOG("Creating Player Application instance");
    return new MPlayerApplication();
}
#endif