//
// MProceduralSkyboxQueue
// Static registry of procedural sky draw calls, polled once per frame by
// MProceduralSkyboxStage — identical pattern to MSkyboxQueue.
//

#pragma once
#ifndef PROCEDURAL_SKYBOX_QUEUE_H
#define PROCEDURAL_SKYBOX_QUEUE_H

#include <vector>

#include "core/object/object.h"

class MProceduralSkyboxDrawCall;

class MProceduralSkyboxQueue : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MProceduralSkyboxDrawCall)
public:
    static void add   (MProceduralSkyboxDrawCall* drawCall);
    static void remove(MProceduralSkyboxDrawCall* drawCall);

    static const std::vector<MProceduralSkyboxDrawCall*>& getAll() { return drawCalls; }

private:
    static std::vector<MProceduralSkyboxDrawCall*> drawCalls;
};

#endif // PROCEDURAL_SKYBOX_QUEUE_H