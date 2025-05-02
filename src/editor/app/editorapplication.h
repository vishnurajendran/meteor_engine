//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#include "editor/meteorite.h"

#ifndef METEOR_ENGINE_EDITORAPPLICATION_H
#define METEOR_ENGINE_EDITORAPPLICATION_H

class MCameraEntity;
class MEditorSceneManager;
class MImGuiSubWindow;
class MImGuiWindow;

class MEditorApplication : public MApplication {
private:
    MEditorSceneManager* sceneManagerRef;
    MObjectPtr<MImGuiWindow> window;
    std::vector<MObjectPtr<MImGuiSubWindow>> subWindows;
    static MEditorApplication *editorInst;

    std::atomic<bool> splashShowing = true;

public:
    static MSpatialEntity* Selected;
public:
    MEditorApplication();
    void initialise() override;
    void run() override;
    void cleanup() override;
    bool isRunning() const override;

    void showSplashScreen();
    void loadPrerequisites();

    //statics
    static void exit();
    static MCameraEntity* getSceneCamera();
};

#endif //METEOR_ENGINE_EDITORAPPLICATION_H
