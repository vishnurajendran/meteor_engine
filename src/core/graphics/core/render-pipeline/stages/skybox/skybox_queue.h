//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef SKYBOX_QUEUE_H
#define SKYBOX_QUEUE_H

#include <vector>
#include "../../../../../object/object.h"

class MSkyboxDrawCall;

// Minimal static registry consumed by MSkyboxStage.
// Skybox entities register their draw call here instead of MRenderQueue
// because the skybox doesn't produce a standard SRenderItem — it has its own
// shader, cubemap binding, and depth state that MSkyboxStage owns entirely.
class MSkyboxQueue : public MObject
{
public:
    static void add   (MSkyboxDrawCall* drawCall);
    static void remove(MSkyboxDrawCall* drawCall);

    static const std::vector<MSkyboxDrawCall*>& getDrawCalls() { return drawCalls; }

private:
    static std::vector<MSkyboxDrawCall*> drawCalls;
};

#endif // SKYBOX_QUEUE_H