//
// capsule_collision_body.h
//

#ifndef CAPSULE_COLLISION_BODY_H
#define CAPSULE_COLLISION_BODY_H

#include "../../../data/shape_axis.h"
#include "core/engine/physics/interface/bodies/collision_body_interface.h"

class ICapsuleCollisionBody : public virtual ICollisionBody
{
public:
    ICapsuleCollisionBody()  = default;
    ~ICapsuleCollisionBody() override = default;

    // Half the height of the cylindrical segment - not the hemisphere caps.
    // Total capsule height along the axis = 2 * (halfHeight + radius).
    virtual void setHalfHeight(float halfHeight) = 0;
    virtual void setRadius(float radius)         = 0;
    virtual void setAxis(EShapeAxis axis)        = 0;
};

#endif // CAPSULE_COLLISION_BODY_H