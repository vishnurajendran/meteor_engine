//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#include "editor/meteorite.h"

#ifndef METEOR_ENGINE_EDITORAPPLICATION_H
#define METEOR_ENGINE_EDITORAPPLICATION_H

class MEditorApplication : public MApplication {
private:
    MObjectPtr<MImGuiWindow> window;
    std::vector<MObjectPtr<MImGuiSubWindow>> subWindows;
public:
    static MSpatialEntity* Selected;
public:
    MEditorApplication();
    void initialise() override;
    void run() override;
    void cleanup() override;
    bool isRunning() const override;
};

#endif //METEOR_ENGINE_EDITORAPPLICATION_H
