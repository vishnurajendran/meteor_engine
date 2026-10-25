//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "application.h"

#include <algorithm>

MApplication* MApplication::appInst = nullptr;

void MApplication::startFrame()
{
    const auto now = std::chrono::steady_clock::now();
    deltaTime = hasLastFrame
        ? std::min(std::chrono::duration<float>(now - lastFrameStart).count(), MAX_DELTA_TIME)
        : 0.0f;
    lastFrameStart = now;
    hasLastFrame   = true;
}
