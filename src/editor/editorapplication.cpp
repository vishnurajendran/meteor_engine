//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorapplication.h"

MEditorApplication::MEditorApplication() : MApplication(){
    name = TEXT("MeteoriteEditor");
}

void MEditorApplication::run() {
    MApplication::run();
    MLOG(TEXT("Run"));
}

void MEditorApplication::cleanup() {
    MApplication::cleanup();
    MLOG(TEXT("Cleanup"));
}

void MEditorApplication::initialise() {
    MApplication::initialise();
    //appRunning = true;
    MLOG(TEXT("Initialise"));
}

#if EDITOR_APPLICATION
MApplication* getAppInstance(){
    MLOG("Creating Editor Application instance");
    return new MEditorApplication();
}
#endif
