//
// Created by ssj5v on 05-10-2024.
//

#include "render_queue.h"
#include "interfaces/drawable_interface.h"

std::vector<IMeteorDrawable*> MRenderQueue::drawables;

void MRenderQueue::addToSubmitables(IMeteorDrawable* instance)
{
    auto it = std::find(drawables.begin(), drawables.end(), instance);
    if (it == drawables.end())
        drawables.push_back(instance);
}

void MRenderQueue::removeFromSubmitables(IMeteorDrawable* instance)
{
    auto it = std::find(drawables.begin(), drawables.end(), instance);
    if (it != drawables.end())
        drawables.erase(it);
}

void MRenderQueue::collectAll(IRenderItemCollector* collector)
{
    for (auto* drawable : drawables)
    {
        if (drawable->canDraw())
            drawable->submitRenderItem(collector);
    }
}