//
// Created by Vishnu Rajendran on 2024-09-24.
//

#pragma once
#ifndef METEOR_ENGINE_APPLICATION_H
#define METEOR_ENGINE_APPLICATION_H
#include "core/meteor_core_minimal.h"
#include "core/engine/scene/scenemanager.h"


class MApplication : public MObject {
protected:
public:
    virtual bool isRunning() const = 0;
    virtual void initialise() = 0;
    virtual void run() = 0;
    virtual void cleanup() = 0;
private:
    MSceneManager* sceneManagerInstance;
};

#endif //METEOR_ENGINE_APPLICATION_H
