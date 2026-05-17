//
// Created by ssj5v on 20-04-2026.
//

#ifndef ENGINE_SETTINGS_H
#define ENGINE_SETTINGS_H

#include "core/object/object.h"
#include "data/serialized_class_base.h"

class MEngineSettings : public SerializedClassBase, public MObject {
    DEFINE_OBJECT_SUBCLASS(MEngineSettings);

    // Window and presentation
    DECLARE_FIELD(resX, int, 800);
    DECLARE_FIELD(resY, int, 600);
    DECLARE_FIELD(fps, int, 60);

    // Shadow
    DECLARE_FIELD(shadowResolution, int, 4096);
};



#endif //ENGINE_SETTINGS_H
