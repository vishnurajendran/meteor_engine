//
// capsule_collision_body_entity.h
//

#ifndef CAPSULE_COLLISION_BODY_ENTITY_H
#define CAPSULE_COLLISION_BODY_ENTITY_H

#include "core/engine/physics/entities/collision_body_entity.h"
#include "core/engine/physics/interface/bodies/capsule/capsule_collision_body.h"
#include "core/engine/physics/data/shape_axis.h"

class MCapsuleCollisionBody : public MCollisionBodyEntity
{
    DEFINE_SPATIAL_CLASS(MCapsuleCollisionBody)

    // halfHeight is half the cylindrical segment — not the hemisphere caps.
    // Total extent along axis = 2 * (halfHeight + radius).
    DECLARE_FIELD(halfHeight, float,      0.5f)
    DECLARE_FIELD(radius,     float,      0.5f)
    DECLARE_FIELD(axis,       EShapeAxis, EShapeAxis::Y)

public:
    MCapsuleCollisionBody()  = default;
    ~MCapsuleCollisionBody() override = default;

    void onDrawGizmo(SVector2 res) override;
    void updateTransforms()        override;

    [[nodiscard]] ICapsuleCollisionBody* getPhysicsBody() const { return physicsBody; }

    static MCapsuleCollisionBody* create(ECollisionBodyType type = ECollisionBodyType::DynamicBody, float halfHeight = 0.5f, float radius = 0.5f, float mass = 10.0f);
    static MCapsuleCollisionBody* createDynamic(float halfHeight = 0.5f, float radius = 0.5f, float mass = 10.0f);
    static MCapsuleCollisionBody* createStatic (float halfHeight = 0.5f, float radius = 0.5f);

protected:
    void createCollisionBody()                               override;
    void releaseBody()                                       override;
    [[nodiscard]] ICollisionBody* getBasePhysicsBody() const override { return physicsBody; }
    void setupShapeCallbacks()                               override;

private:
    void syncShape();
    void axisScaleFactors(float& outScaleR, float& outScaleH, const SVector3& scale) const;

    ICapsuleCollisionBody* physicsBody = nullptr;
};

#endif // CAPSULE_COLLISION_BODY_ENTITY_H