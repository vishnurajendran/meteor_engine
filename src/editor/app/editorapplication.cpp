//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorapplication.h"

#include "core/graphics/core/meteordrawables.h"

MSpatialEntity* MEditorApplication::Selected = nullptr;

MEditorApplication::MEditorApplication() : MApplication(){
    name = STR("MeteoriteEditor");
}

void MEditorApplication::run() {
    if(window == nullptr)
        return;

    window->clear();

    //Todo: set real delta time here
    MSceneManager::update(0.0);

    MMeteorDrawables::requestDrawCalls();
    window->update();
}

void MEditorApplication::cleanup() {
    MLOG(STR("Cleanup"));
    window->close();
    window = nullptr;
}

void MEditorApplication::initialise() {
    //appRunning = true;
    MLOG(STR("Initialising Editor"));
    window = (MImGuiWindow*)MWindowManager::getImGuiWindow(STR("Meteorite Editor"));
    if(window == nullptr){
        MERROR(STR("Invalid Window Type"));
        return;
    }
    if(!window->isOpen()) {
        MERROR(STR("Failed to open window"));
        return;
    }

    // Load first, so that it can log everything from here on out.
    subWindows.push_back(new MEditorConsoleWindow());

    //Refresh Asset Manager
    MAssetManager::getInstance()->refresh();

    subWindows.push_back(new MEditorHierarchyWindow());
    subWindows.push_back(new MEditorInspectorWindow());
    subWindows.push_back(new MEditorSceneViewWindow());
    MLOG(STR("Loaded Editor Windows"));
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
