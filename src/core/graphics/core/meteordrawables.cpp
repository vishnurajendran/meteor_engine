//
// Created by ssj5v on 05-10-2024.
//
#include "meteordrawables.h"

#include "drawcallsubmitable.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"

std::vector<IDrawCallSubmitable*> MMeteorDrawables::drawables;

void MMeteorDrawables::addToSubmitables(IDrawCallSubmitable *instance) {
    auto it = std::find(drawables.begin(), drawables.end(), instance);
    if(it == drawables.end()) {
        drawables.push_back(instance);
    }
}

void MMeteorDrawables::removeFromSubmitables(IDrawCallSubmitable *instance) {
    auto it = std::find(drawables.begin(), drawables.end(), instance);
    if(it != drawables.end()) {
        drawables.erase(it);
    }
}

void MMeteorDrawables::requestDrawCalls() {
    for(auto drawable : drawables) {
        drawable->raiseDrawCall();
    }
}
