//
// Created by ssj5v on 05-10-2024.
//

#pragma once
#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H

#include <vector>
#include "../../../object/object.h"


class IRenderItemCollector;
class IMeteorDrawable;

// Every drawable in the world. Owned by the render pipeline manager:
//   MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>()->getRenderQueue()
class MRenderQueue : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MRenderQueue)
public:
    void addToSubmitables     (IMeteorDrawable* instance);
    void removeFromSubmitables(IMeteorDrawable* instance);

    // Called once per frame by MRenderPipeline::preRender().
    // Every drawable that canDraw() submits its SRenderItem(s) to the collector.
    void collectAll(IRenderItemCollector* collector);

private:
    std::vector<IMeteorDrawable*> drawables;
};

#endif // RENDERQUEUE_H