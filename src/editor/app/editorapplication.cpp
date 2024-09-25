//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorapplication.h"

MEditorApplication::MEditorApplication() : MApplication(){
    name = TEXT("MeteoriteEditor");
}

void MEditorApplication::run() {
    if(window == nullptr)
        return;

    window->clear();
    window->update();
}

void MEditorApplication::cleanup() {
    MLOG(TEXT("Cleanup"));
    window->close();
    window = nullptr;
}

void MEditorApplication::initialise() {
    //appRunning = true;
    MLOG(TEXT("Initialising Editor"));
    window = MWindowManager::getImGuiWindow(TEXT("Meteorite Editor"));
    if(!window->isOpen())
        MERROR(TEXT("Failed to open window"));

    subWindows.push_back(new MEditorConsoleWindow());
    MLOG(TEXT("Loaded Editor Windows"));
}

bool MEditorApplication::isRunning() const {
    if(window == nullptr)
        return false;

    return window->isOpen();
}

#if EDITOR_APPLICATION
MApplication* getAppInstance(){
    MLOG("Creating Editor Application instance");
    return new MEditorApplication();
}
#endif
