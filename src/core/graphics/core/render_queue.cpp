//
// Created by ssj5v on 05-10-2024.
//
#include "render_queue.h"

#include "drawable_interface.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"

std::vector<IMeteorDrawable*> MRenderQueue::drawables;

void MRenderQueue::addToSubmitables(IMeteorDrawable *instance) {
    auto it = std::find(drawables.begin(), drawables.end(), instance);
    if(it == drawables.end()) {
        drawables.push_back(instance);
    }
}

void MRenderQueue::removeFromSubmitables(IMeteorDrawable *instance) {
    auto it = std::find(drawables.begin(), drawables.end(), instance);
    if(it != drawables.end()) {
        drawables.erase(it);
    }
}

void MRenderQueue::requestDrawCalls() {
    for(auto drawable : drawables) {
        if (drawable->canDraw())
            drawable->raiseDrawCall();
    }
}

void MRenderQueue::prepareToDraw()
{
    // inform all drawables to prepare for rendering.
    for(auto drawable : drawables) {
        if (drawable->canDraw())
            drawable->prepareForDraw();
    }
}
