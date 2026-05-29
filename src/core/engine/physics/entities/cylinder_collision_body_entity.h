//
// cylinder_collision_body_entity.h
//

#ifndef CYLINDER_COLLISION_BODY_ENTITY_H
#define CYLINDER_COLLISION_BODY_ENTITY_H

#include "core/engine/physics/entities/collision_body_entity.h"
#include "core/engine/physics/interface/bodies/cylinder/cylinder_collision_body.h"
#include "core/engine/physics/data/shape_axis.h"

class MCylinderCollisionBody : public MCollisionBodyEntity
{
    DEFINE_SPATIAL_CLASS(MCylinderCollisionBody)

    DECLARE_FIELD(halfHeight, float,      0.5f)
    DECLARE_FIELD(radius,     float,      0.5f)
    DECLARE_FIELD(axis,       EShapeAxis, EShapeAxis::Y)

public:
    MCylinderCollisionBody()  = default;
    ~MCylinderCollisionBody() override = default;

    void onDrawGizmo(SVector2 res) override;
    void updateTransforms()        override;

    [[nodiscard]] ICylinderCollisionBody* getPhysicsBody() const { return physicsBody; }

    static MCylinderCollisionBody* create(ECollisionBodyType type = ECollisionBodyType::StaticBody, float halfHeight = 0.5f, float radius = 0.5f, float mass = 10.0f);
    static MCylinderCollisionBody* createDynamic(float halfHeight = 0.5f, float radius = 0.5f, float mass = 10.0f);
    static MCylinderCollisionBody* createStatic (float halfHeight = 0.5f, float radius = 0.5f);

protected:
    void createCollisionBody()                               override;
    void releaseBody()                                       override;
    [[nodiscard]] ICollisionBody* getBasePhysicsBody() const override { return physicsBody; }
    void setupShapeCallbacks()                               override;

private:
    void syncShape();
    void axisScaleFactors(float& outScaleR, float& outScaleH, const SVector3& scale) const;

    ICylinderCollisionBody* physicsBody = nullptr;
};

#endif // CYLINDER_COLLISION_BODY_ENTITY_H