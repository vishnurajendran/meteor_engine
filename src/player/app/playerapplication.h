//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once

#ifndef METEOR_ENGINE_PLAYERAPPLICATION_H
#define METEOR_ENGINE_PLAYERAPPLICATION_H
#include "core/meteor_core.h"

enum EPlayerApplicationState
{
    Playing,
    Paused
};

class MPlayerApplication : public MApplication {
public:
    // application is always in play-state
    [[nodiscard]] bool isPlaying() const override { return playerState == EPlayerApplicationState::Playing; }
    [[nodiscard]] SString getEngineSettingsPath() const override { return "PlayerSettings.xml"; }

    void pause(const bool& pause) override { playerState = EPlayerApplicationState::Playing; }
    [[nodiscard]] bool isPaused() const override { return playerState == EPlayerApplicationState::Paused; };

private:
    DEFINE_OBJECT_SUBCLASS(MPlayerApplication)
private:
    MObjectPtr<MWindow> window;
    MRenderPipelineManager pipelineManager;
public:
    MPlayerApplication();
    void initialise() override;
    void run() override;
    void cleanup() override;
    [[nodiscard]] bool isRunning() const override;

private:
    EPlayerApplicationState playerState = EPlayerApplicationState::Playing;
};
#endif //METEOR_ENGINE_PLAYERAPPLICATION_H
