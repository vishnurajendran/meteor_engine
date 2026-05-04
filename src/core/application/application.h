//
// Created by Vishnu Rajendran on 2024-09-24.
//

#pragma once
#ifndef METEOR_ENGINE_APPLICATION_H
#define METEOR_ENGINE_APPLICATION_H
#include "core/engine/scene/scenemanager.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager.h"
#include "core/meteor_core_minimal.h"


class MApplication : public MObject {
public:
    virtual bool isRunning() const = 0;
    virtual void initialise() = 0;
    virtual void run() = 0;
    virtual void cleanup() = 0;
    virtual SString getEngineSettingsPath() const = 0;

private:
    MSceneManager* sceneManagerInstance = nullptr;
    float startTime = 0;

protected:
    float deltaTime = 0.0f;
    void startFrame();
    void endFrame();
};

#endif //METEOR_ENGINE_APPLICATION_H
