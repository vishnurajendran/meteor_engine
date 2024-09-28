//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorapplication.h"

MSpatialEntity* MEditorApplication::Selected = nullptr;

MEditorApplication::MEditorApplication() : MApplication(){
    name = STR("MeteoriteEditor");
}

void MEditorApplication::run() {
    if(window == nullptr)
        return;

    window->clear();
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
    subWindows.push_back(new MEditorConsoleWindow());
    subWindows.push_back(new MEditorHierarchyWindow());
    subWindows.push_back(new MEditorInspectorWindow());
    subWindows.push_back(new MEditorSceneViewWindow());
    MLOG(STR("Loaded Editor Windows"));

    MSceneManager::loadScene(STR("assets/scenes/test_scene.scml"));
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
