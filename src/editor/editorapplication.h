//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#include "core/meteor_core.h"

#ifndef METEOR_ENGINE_EDITORAPPLICATION_H
#define METEOR_ENGINE_EDITORAPPLICATION_H

class MEditorApplication : public MApplication {
public:
    MEditorApplication();
    void initialise() override;
    void run() override;
    void cleanup() override;
};

#endif //METEOR_ENGINE_EDITORAPPLICATION_H
