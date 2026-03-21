//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "application.h"

void MApplication::startFrame()
{
    startTime = std::clock();
}

void MApplication::endFrame()
{
    deltaTime = (std::clock() - startTime)/static_cast<float>(CLOCKS_PER_SEC);
}
