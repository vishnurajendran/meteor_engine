//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef SKYBOX_QUEUE_H
#define SKYBOX_QUEUE_H

#include <vector>
#include "../../../../../object/object.h"

class MSkyboxDrawCall;

// Skybox draw calls, consumed by MSkyboxStage. Owned by the render pipeline
// manager (getSkyboxQueue()).
// Skybox entities register their draw call here instead of MRenderQueue
// because the skybox doesn't produce a standard SRenderItem - it has its own
// shader, cubemap binding, and depth state that MSkyboxStage owns entirely.
class MSkyboxQueue : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MSkyboxQueue)
public:
    void add   (MSkyboxDrawCall* drawCall);
    void remove(MSkyboxDrawCall* drawCall);

    const std::vector<MSkyboxDrawCall*>& getDrawCalls() const { return drawCalls; }

private:
    std::vector<MSkyboxDrawCall*> drawCalls;
};

#endif // SKYBOX_QUEUE_H