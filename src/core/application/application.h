//
// Created by Vishnu Rajendran on 2024-09-24.
//

#pragma once
#ifndef METEOR_ENGINE_APPLICATION_H
#define METEOR_ENGINE_APPLICATION_H
#include "core/default_settings_paths.h"
#include "core/engine/scene/scenemanager.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager.h"

/// Defines the application for the meteor
SCRIPT_BIND_CLASS()
class MApplication : public MObject {
    DEFINE_OBJECT_SUBCLASS(MApplication)
public:
    MApplication() = default;
    virtual void initialise() { appInst = this; };
    virtual void run() = 0;
    virtual void cleanup() = 0;

    SCRIPT_BIND_FUNC()
    virtual void pause(const bool& pause) = 0;

    [[nodiscard]] virtual SString getEngineSettingsPath() const { return SString::format("{0}{1}", DEFAULT_SETTINGS_PATH, "EngineSettings.xml"); }

    /**
     * this is always true application is closed.
     * DO NOT USE THIS... ITS USED INTERNALLY AND DOES NOT REPRESENT SYSTEM STATE
     * @return true when the app is running.
     */
    [[nodiscard]] virtual bool isRunning() const = 0;

    /**
     * can be false even if simulating is true
     * @return true when application is paused
     */
    SCRIPT_BIND_FUNC()
    [[nodiscard]] virtual bool isPaused() const  = 0;

    /**
     * Playing depicts if the application is actively playing
     * @return true when application is playing, playing can be false even simulation is true
     */
    SCRIPT_BIND_FUNC()
    [[nodiscard]] virtual bool isPlaying() const = 0;

    /**
     * During simulation, scripts, audio and physics are active and running
     * @return true when simulating
     */
    [[nodiscard]] virtual bool isSimulating() const = 0;

    SCRIPT_BIND_FUNC_STATIC()
    static MApplication* getAppInstance() { return appInst; };

private:
    float startTime = 0;

protected:
    void startFrame();
    void endFrame();

protected:
    float deltaTime = 0.0f;
    static MApplication* appInst;
};
#endif //METEOR_ENGINE_APPLICATION_H
