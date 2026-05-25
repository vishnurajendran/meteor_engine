//
// Created by Vishnu Rajendran on 2024-09-24.
//

#pragma once
#ifndef METEOR_ENGINE_APPLICATION_H
#define METEOR_ENGINE_APPLICATION_H
#include "core/engine/scene/scenemanager.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager.h"
#include "core/meteor_core_minimal.h"

/// Defines the application for the meteor
class MApplication : public MObject {
    DEFINE_OBJECT_SUBCLASS(MApplication)
public:
    MApplication() = default;
    virtual void initialise() { appInst = this; };
    virtual void run() = 0;
    virtual void cleanup() = 0;
    virtual void pause(const bool& pause) = 0;

    [[nodiscard]] virtual bool isRunning() const = 0;
    [[nodiscard]] virtual SString getEngineSettingsPath() const = 0;
    [[nodiscard]] virtual bool isPaused() const  = 0;
    [[nodiscard]] virtual bool isPlaying() const = 0;

    static MApplication* getAppInstance() { return appInst; };

private:
    MSceneManager* sceneManagerInstance = nullptr;
    float startTime = 0;

protected:
    void startFrame();
    void endFrame();

protected:
    float deltaTime = 0.0f;
    static MApplication* appInst;
};
#endif //METEOR_ENGINE_APPLICATION_H
