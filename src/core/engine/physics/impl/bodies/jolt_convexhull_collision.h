//
// jolt_convex_hull_collision.h
//

#ifndef JOLT_CONVEX_HULL_COLLISION_H
#define JOLT_CONVEX_HULL_COLLISION_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "core/engine/physics/impl/jolt_body_interface.h"
#include "core/engine/physics/interface/bodies/convexhull/convexhull_collsion_body.h"
#include "core/object/object.h"
#include "core/object/object_class_macros.h"
#include "macros/jolt_base_collision_body_macros.h"

class MJoltConvexHullCollision : public MObject, public IConvexHullCollisionBody, public IJoltCollisionBodyInterface
{
    DEFINE_OBJECT_SUBCLASS(MJoltConvexHullCollision)
    DECLARE_BASE_COLLISION_BODY()
public:
    MJoltConvexHullCollision(JPH::BodyID         bodyId,
                              JPH::BodyInterface& bodyInterface,
                              JPH::PhysicsSystem& physicsSystem);

    ~MJoltConvexHullCollision() override = default;

    bool setPoints(const std::vector<SVector3>& points) override;
};

#endif // JOLT_CONVEX_HULL_COLLISION_H