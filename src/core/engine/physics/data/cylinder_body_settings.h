//
// cylinder_body_settings.h
//

#ifndef CYLINDER_BODY_SETTINGS_H
#define CYLINDER_BODY_SETTINGS_H

#include "core/engine/physics/data/default_body_settings.h"
#include "core/engine/physics/interface/bodies/shape_axis.h"

struct SCylinderBodySettings
{
    SVector3           position          = {};
    SQuaternion        rotation          = {};
    ECollisionBodyType bodyType          = ECollisionBodyType::StaticBody;
    float              mass              = 10.0f;
    bool               affectedByGravity = false;

    float      halfHeight = 0.5f;
    float      radius     = 0.5f;
    EShapeAxis axis       = EShapeAxis::Y;
};

#endif // CYLINDER_BODY_SETTINGS_H