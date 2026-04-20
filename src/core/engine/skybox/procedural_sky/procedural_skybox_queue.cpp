//
// MProceduralSkyboxQueue
//

#include "procedural_skybox_queue.h"
#include <algorithm>

std::vector<MProceduralSkyboxDrawCall*> MProceduralSkyboxQueue::drawCalls;

void MProceduralSkyboxQueue::add(MProceduralSkyboxDrawCall* drawCall)
{
    auto it = std::find(drawCalls.begin(), drawCalls.end(), drawCall);
    if (it == drawCalls.end())
        drawCalls.push_back(drawCall);
}

void MProceduralSkyboxQueue::remove(MProceduralSkyboxDrawCall* drawCall)
{
    auto it = std::find(drawCalls.begin(), drawCalls.end(), drawCall);
    if (it != drawCalls.end())
        drawCalls.erase(it);
}