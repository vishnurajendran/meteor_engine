//
// Created by Vishnu Rajendran on 2024-09-24.
//

#pragma once
#ifndef METEOR_ENGINE_APPLICATION_H
#define METEOR_ENGINE_APPLICATION_H
#include "core/meteor_core.h"

class MApplication : public MObject {
protected:
    bool appRunning;
public:
    MApplication();
    bool isRunning() const;
    virtual void initialise();
    virtual void run();
    virtual void cleanup();
};

#endif //METEOR_ENGINE_APPLICATION_H
