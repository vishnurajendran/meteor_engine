//
// Created by ssj5v on 15-04-2026.
//

#include "skybox_queue.h"
#include "skyboxdrawcall.h"

std::vector<MSkyboxDrawCall*> MSkyboxQueue::drawCalls;

void MSkyboxQueue::add(MSkyboxDrawCall* drawCall)
{
    auto it = std::find(drawCalls.begin(), drawCalls.end(), drawCall);
    if (it == drawCalls.end())
        drawCalls.push_back(drawCall);
}

void MSkyboxQueue::remove(MSkyboxDrawCall* drawCall)
{
    auto it = std::find(drawCalls.begin(), drawCalls.end(), drawCall);
    if (it != drawCalls.end())
        drawCalls.erase(it);
}