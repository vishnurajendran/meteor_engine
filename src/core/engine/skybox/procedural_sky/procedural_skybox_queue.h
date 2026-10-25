//
// MProceduralSkyboxQueue
// Procedural sky draw calls, polled once per frame by MProceduralSkyboxStage -
// identical pattern to MSkyboxQueue. Owned by the render pipeline manager
// (getProceduralSkyboxQueue()).
//

#pragma once
#ifndef PROCEDURAL_SKYBOX_QUEUE_H
#define PROCEDURAL_SKYBOX_QUEUE_H

#include <vector>

#include "core/object/object.h"

class MProceduralSkyboxDrawCall;

class MProceduralSkyboxQueue : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MProceduralSkyboxQueue)
public:
    void add   (MProceduralSkyboxDrawCall* drawCall);
    void remove(MProceduralSkyboxDrawCall* drawCall);

    const std::vector<MProceduralSkyboxDrawCall*>& getAll() const { return drawCalls; }

private:
    std::vector<MProceduralSkyboxDrawCall*> drawCalls;
};

#endif // PROCEDURAL_SKYBOX_QUEUE_H