//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once

#ifndef METEOR_ENGINE_PLAYERAPPLICATION_H
#define METEOR_ENGINE_PLAYERAPPLICATION_H
#include "core/meteor_core.h"

class MPlayerApplication : public MApplication {
private:
    MObjectPtr<MWindow> window;
public:
    MPlayerApplication();
    void initialise() override;
    void run() override;
    void cleanup() override;
    bool isRunning() const override;
};
#endif //METEOR_ENGINE_PLAYERAPPLICATION_H
