//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorapplication.h"

#include "core/graphics/core/render_queue.h"
#include "editor/editorscenemanager/editorscenemanager.h"
#include "editor/window/menubar/menubartree.h"
#include "editor/window/imgui/imguiwindow.h"


MSpatialEntity* MEditorApplication::Selected = nullptr;
MEditorApplication* MEditorApplication::editorInst = nullptr;


MEditorApplication::MEditorApplication() : MApplication(){
    name = STR("MeteoriteEditor");
}

void MEditorApplication::run() {
    if(window == nullptr)
        return;

    window->clear();

    //Todo: set real delta time here
    MSceneManager::getSceneManagerInstance()->update(0.0);
    window->update();
}

void MEditorApplication::cleanup() {
    MLOG(STR("Cleanup..."));

    MLOG(STR("Deleting SceneManager"));
    delete sceneManagerRef;

    MLOG(STR("Closing Window"));
    if (window != nullptr)
        window->close();
    window = nullptr;
    MLOG(STR("Editor Application Cleanup Complete"));
}

void MEditorApplication::initialise() {
    //appRunning = true;
    MLOG(STR("Initialising Editor"));
    window = new MImGuiWindow(STR("Meteorite Editor"));
    if(window == nullptr){
        MERROR(STR("Invalid Window Type"));
        return;
    }
    if(!window->isOpen()) {
        MERROR(STR("Failed to open window"));
        return;
    }

    editorInst = this;
    // Load first, so that it can log everything from here on out.
    subWindows.push_back(new MEditorConsoleWindow());

    //Refresh Asset Manager
    MAssetManager::getInstance()->refresh();

    //setup scene manager
    sceneManagerRef = new MEditorSceneManager();
    MSceneManager::registerSceneManager(sceneManagerRef);

    //add remaining windows
    subWindows.push_back(new MEditorHierarchyWindow());
    subWindows.push_back(new MEditorInspectorWindow());
    subWindows.push_back(new MEditorSceneViewWindow());
    MLOG(STR("Loaded Editor Windows"));

    MMenubarTreeNode::buildTree();
    MLOG(STR("Built Menubar Items"));
}

bool MEditorApplication::isRunning() const {
    if(window == nullptr)
        return false;

    return window->isOpen();
}

void MEditorApplication::exit()
{
    if (editorInst != nullptr)
        editorInst->window->close();
}

MCameraEntity* MEditorApplication::getSceneCamera()
{
    if (editorInst == nullptr)
        return nullptr;

    return editorInst->sceneManagerRef->getEditorSceneCamera();
}

#if EDITOR_APPLICATION
MApplication* getAppInstance(){
    MLOG("Creating Editor Application instance");
    return new MEditorApplication();
}
#endif
