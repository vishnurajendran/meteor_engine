//
// Created by ssj5v on 20-04-2026.
//

#ifndef ENGINE_SETTINGS_H
#define ENGINE_SETTINGS_H

#include "core/object/object.h"
#include "data/serialized_class_base.h"
#include "core/utils/field_engine_types.h"
class MEngineSettings : public SerializedClassBase, public MObject {
    DEFINE_OBJECT_SUBCLASS(MEngineSettings);

    // Window and presentation
    DECLARE_FIELD(resX, int, 800);
    DECLARE_FIELD(resY, int, 600);
    DECLARE_FIELD(fps, int, 60);

    // Shadow
    DECLARE_FIELD(shadowResolution, int, 4096);

    // Physics
    DECLARE_FIELD(maxPhysicsTempAllocSize, int, 1024 * 1024 * 10);
    DECLARE_FIELD(physicsTickRate, float, 60.0f);
    DECLARE_FIELD(maxPhysicsBodies, int, 1024);
    DECLARE_FIELD(numPhysicsBodyMutexes, int, 0); // no-limit
    DECLARE_FIELD(maxPhysicsBodyPairs, int, 1024);
    DECLARE_FIELD(maxPhysicsContactConstraints, int, 1024);
    DECLARE_FIELD(gravity, SVector3, SVector3(0.0f, -9.8f, 0.0f));
};



#endif //ENGINE_SETTINGS_H
