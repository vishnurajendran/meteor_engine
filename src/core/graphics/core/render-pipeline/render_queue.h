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

class MRenderQueue : public MObject
{
public:
    static void addToSubmitables     (IMeteorDrawable* instance);
    static void removeFromSubmitables(IMeteorDrawable* instance);

    // Called once per frame by MRenderPipeline::preRender().
    // Every drawable that canDraw() submits its SRenderItem(s) to the collector.
    static void collectAll(IRenderItemCollector* collector);

private:
    static std::vector<IMeteorDrawable*> drawables;
};

#endif // RENDERQUEUE_H