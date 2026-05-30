//
// jolt_mesh_collision.h
//

#ifndef JOLT_MESH_COLLISION_H
#define JOLT_MESH_COLLISION_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "core/engine/physics/impl/jolt_body_interface.h"
#include "core/engine/physics/interface/bodies/mesh/mesh_collision_body.h"
#include "core/object/object.h"
#include "core/object/object_class_macros.h"
#include "macros/jolt_base_collision_body_macros.h"

class MJoltMeshCollision : public MObject, public IMeshCollisionBody, public IJoltCollisionBodyInterface
{
    DEFINE_OBJECT_SUBCLASS(MJoltMeshCollision)
    DECLARE_BASE_COLLISION_BODY()
public:
    MJoltMeshCollision(JPH::BodyID         bodyId,
                       JPH::BodyInterface& bodyInterface,
                       JPH::PhysicsSystem& physicsSystem);

    ~MJoltMeshCollision() override = default;

    bool setMeshData(const std::vector<SVector3>& vertices,
                     const std::vector<uint32_t>& indices) override;
};

#endif // JOLT_MESH_COLLISION_H