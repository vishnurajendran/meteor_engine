//
// cylinder_collision_body.h
//

#ifndef CYLINDER_COLLISION_BODY_H
#define CYLINDER_COLLISION_BODY_H

#include "../../../data/shape_axis.h"
#include "core/engine/physics/interface/bodies/collision_body_interface.h"

class ICylinderCollisionBody : public virtual ICollisionBody
{
public:
    ICylinderCollisionBody()  = default;
    ~ICylinderCollisionBody() override = default;

    // Half the total height along the cylinder axis.
    virtual void setHalfHeight(float halfHeight) = 0;
    virtual void setRadius(float radius)         = 0;

    // Changing the axis rebuilds the underlying shape - prefer setting it once
    // at creation time rather than changing it at runtime.
    virtual void setAxis(EShapeAxis axis) = 0;
};

#endif // CYLINDER_COLLISION_BODY_H