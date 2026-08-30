//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once

#ifndef METEOR_ENGINE_PLAYERAPPLICATION_H
#define METEOR_ENGINE_PLAYERAPPLICATION_H
#include "core/application/application.h"
#include "core/object/objectpointer.h"
#include "core/window/simple/windowing.h"


class IPhysicsEngineSubsystem;
enum EPlayerApplicationState
{
    Playing,
    Paused
};

class MPlayerApplication : public MApplication {
public:
    // application is always in play-state
    [[nodiscard]] bool isSimulating() const override { return true; };
    [[nodiscard]] bool isPlaying() const override { return playerState == EPlayerApplicationState::Playing; }
    [[nodiscard]] bool isPaused() const override { return playerState == EPlayerApplicationState::Paused; };
    [[nodiscard]] SString getEngineSettingsPath() const override { return "PlayerSettings.xml"; }

    void pause(const bool& pause) override;

private:
    DEFINE_OBJECT_SUBCLASS(MPlayerApplication)

private:
    void registerSubsystems();

public:
    MPlayerApplication();
    void initialise() override;
    void run() override;
    float getPhysicsStep() const;
    void tickPhysics(float deltaTime);
    void cleanup() override;
    [[nodiscard]] bool isRunning() const override;

private:
    EPlayerApplicationState playerState = EPlayerApplicationState::Playing;
    MObjectPtr<MWindow> window = nullptr;
    IRenderPipelineManagerSubsystem* pipelineManager= nullptr;
    IPhysicsEngineSubsystem* physicsEngineRef = nullptr;
    float physicsAccumulator = 0.0f;
    bool internal_tickSpatialFixedUpdateFlag   = false;
    MSceneManager* sceneManagerRef = nullptr;
};
#endif //METEOR_ENGINE_PLAYERAPPLICATION_H
