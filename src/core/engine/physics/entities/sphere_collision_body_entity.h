//
// sphere_collision_body_entity.h
//

#ifndef SPHERE_COLLISION_BODY_ENTITY_H
#define SPHERE_COLLISION_BODY_ENTITY_H

#include "core/engine/physics/entities/collision_body_entity.h"
#include "core/engine/physics/interface/bodies/sphere/sphere_collision_body.h"

class MSphereCollisionBody : public MCollisionBodyEntity
{
    DEFINE_SPATIAL_CLASS(MSphereCollisionBody)

    DECLARE_FIELD(radius, float, 1.0f)

public:
    MSphereCollisionBody()  = default;
    ~MSphereCollisionBody() override = default;

    void onDrawGizmo(SVector2 res) override;
    void updateTransforms()        override;

    [[nodiscard]] ISphereCollisionBody* getPhysicsBody() const { return physicsBody; }

    static MSphereCollisionBody* create(ECollisionBodyType type = ECollisionBodyType::StaticBody, float radius = 1.0f, float mass = 10.0f);
    static MSphereCollisionBody* createDynamic(float radius = 1.0f, float mass = 10.0f);
    static MSphereCollisionBody* createStatic (float radius = 1.0f);
    static MSphereCollisionBody* createTrigger(float radius = 1.0f);

protected:
    void createCollisionBody()                                   override;
    void releaseBody()                                           override;
    [[nodiscard]] ICollisionBody* getBasePhysicsBody() const     override { return physicsBody; }
    void setupShapeCallbacks()                                   override;

private:
    void syncRadius();

    ISphereCollisionBody* physicsBody = nullptr;
};

#endif // SPHERE_COLLISION_BODY_ENTITY_H