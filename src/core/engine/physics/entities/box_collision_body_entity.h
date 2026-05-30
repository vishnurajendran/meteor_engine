//
// box_collision_body_entity.h
//

#ifndef BOX_COLLISION_BODY_ENTITY_H
#define BOX_COLLISION_BODY_ENTITY_H

#include "core/engine/physics/entities/collision_body_entity.h"
#include "core/engine/physics/interface/bodies/box/box_collision_body.h"

class MBoxCollisionBody : public MCollisionBodyEntity
{
    DEFINE_SPATIAL_CLASS(MBoxCollisionBody)

    DECLARE_FIELD(halfExtentX, float, 0.5f)
    DECLARE_FIELD(halfExtentY, float, 0.5f)
    DECLARE_FIELD(halfExtentZ, float, 0.5f)

public:
    MBoxCollisionBody()  = default;
    ~MBoxCollisionBody() override = default;

    void onDrawGizmo(SVector2 res) override;
    void updateTransforms()        override;

    [[nodiscard]] IBoxCollisionBody* getPhysicsBody() const { return physicsBody; }

    static MBoxCollisionBody* create(ECollisionBodyType type        = ECollisionBodyType::StaticBody,
                                     SVector3           halfExtents = { 0.5f, 0.5f, 0.5f },
                                     float              mass        = 10.0f);
    static MBoxCollisionBody* createDynamic(SVector3 halfExtents = { 0.5f, 0.5f, 0.5f }, float mass = 10.0f);
    static MBoxCollisionBody* createStatic (SVector3 halfExtents = { 0.5f, 0.5f, 0.5f });
    static MBoxCollisionBody* createTrigger(SVector3 halfExtents = { 0.5f, 0.5f, 0.5f });

protected:
    void createCollisionBody()                           override;
    void releaseBody()                                   override;
    [[nodiscard]] ICollisionBody* getBasePhysicsBody() const override { return physicsBody; }
    void setupShapeCallbacks()                           override;

private:
    void syncBounds();

    IBoxCollisionBody* physicsBody = nullptr;
};

#endif // BOX_COLLISION_BODY_ENTITY_H