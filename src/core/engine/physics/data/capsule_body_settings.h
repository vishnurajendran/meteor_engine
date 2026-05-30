//
// capsule_body_settings.h
//

#ifndef CAPSULE_BODY_SETTINGS_H
#define CAPSULE_BODY_SETTINGS_H

#include "core/engine/physics/data/default_body_settings.h"
#include "shape_axis.h"

struct SCapsuleBodySettings
{
    SVector3           position          = {};
    SQuaternion        rotation          = {};
    ECollisionBodyType bodyType          = ECollisionBodyType::DynamicBody;
    float              mass              = 10.0f;
    bool               affectedByGravity = true;

    // halfHeight is half the cylindrical segment, not including the hemisphere caps.
    // Total height along the axis = 2 * (halfHeight + radius).
    float      halfHeight = 0.5f;
    float      radius     = 0.5f;
    EShapeAxis axis       = EShapeAxis::Y;
};

#endif // CAPSULE_BODY_SETTINGS_H