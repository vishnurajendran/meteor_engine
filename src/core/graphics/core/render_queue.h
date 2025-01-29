//
// Created by ssj5v on 05-10-2024.
//

#ifndef RENDERQUEUE_H
#define RENDERQUEUE_H
#include <vector>
#include "core/object/object.h"

class IMeteorDrawable;

class MRenderQueue : public MObject {
private:
    static std::vector<IMeteorDrawable*> drawables;
public:
    static void addToSubmitables(IMeteorDrawable* instance);
    static void removeFromSubmitables(IMeteorDrawable* instance);
    static void requestDrawCalls();
    static void prepareToDraw();
};

#endif //RENDERQUEUE_H
