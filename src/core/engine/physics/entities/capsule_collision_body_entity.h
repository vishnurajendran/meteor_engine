//
// capsule_collision_body_entity.h
//

#ifndef CAPSULE_COLLISION_BODY_ENTITY_H
#define CAPSULE_COLLISION_BODY_ENTITY_H

#include <functional>

#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/physics/callback/physics_callback_reciever.h"
#include "core/engine/physics/data/collision_data.h"
#include "core/engine/physics/interface/bodies/capsule/capsule_collision_body.h"
#include "core/engine/physics/interface/physics_engine_interface.h"


class MCapsuleCollisionBody : public MSpatialEntity, public IPhysicsCallbackReceiver
{
    DEFINE_SPATIAL_CLASS(MCapsuleCollisionBody)

    DECLARE_FIELD(bodyType,          ECollisionBodyType, ECollisionBodyType::DynamicBody)
    DECLARE_FIELD(mass,              float,              10.0f)
    DECLARE_FIELD(affectedByGravity, bool,               true)
    DECLARE_FIELD(gravityScale,      float,              1.0f)
    DECLARE_FIELD(isSensor,          bool,               false)
    DECLARE_FIELD(linearDamping,     float,              0.0f)
    DECLARE_FIELD(angularDamping,    float,              0.0f)
    DECLARE_FIELD(restitution,       float,              0.3f)
    DECLARE_FIELD(friction,          float,              0.6f)
    // halfHeight is the half-length of the cylindrical segment — not including caps.
    // Total extent along axis = 2 * (halfHeight + radius).
    DECLARE_FIELD(halfHeight,        float,              0.5f)
    DECLARE_FIELD(radius,            float,              0.5f)
    DECLARE_FIELD(axis,              EShapeAxis,         EShapeAxis::Y)

public:
    MCapsuleCollisionBody()  = default;
    ~MCapsuleCollisionBody() override = default;

    void onCreate()                override;
    void onStart()                 override;
    void onUpdate(float deltaTime) override;
    void onExit()                  override;
    void onDrawGizmo(SVector2 res) override;
    void updateTransforms()        override;

    // ---- Scene creation helpers --------------------------------------------

    static MCapsuleCollisionBody* create(ECollisionBodyType type       = ECollisionBodyType::DynamicBody,
                                         float              halfHeight = 0.5f,
                                         float              radius     = 0.5f,
                                         float              mass       = 10.0f);

    static MCapsuleCollisionBody* createDynamic(float halfHeight = 0.5f, float radius = 0.5f, float mass = 10.0f);
    static MCapsuleCollisionBody* createStatic (float halfHeight = 0.5f, float radius = 0.5f);

    // ---- IPhysicsCallbackReceiver ------------------------------------------
    [[nodiscard]] MSpatialEntity* getEntity() override { return this; }
    void dispatchCollisionStart(const SCollisionData& data) override;
    void dispatchCollisionStay (const SCollisionData& data) override;
    void dispatchCollisionEnd  (const SCollisionData& data) override;
    void dispatchTriggerStart  (const SOverlapData&   data) override;
    void dispatchTriggerStay   (const SOverlapData&   data) override;
    void dispatchTriggerEnd    (const SOverlapData&   data) override;

    // ---- Callback registration ---------------------------------------------
    void setOnCollisionStart(std::function<void(const SCollisionData&)> cb) { onCollisionStartCb = std::move(cb); }
    void setOnCollisionStay (std::function<void(const SCollisionData&)> cb) { onCollisionStayCb  = std::move(cb); }
    void setOnCollisionEnd  (std::function<void(const SCollisionData&)> cb) { onCollisionEndCb   = std::move(cb); }
    void setOnTriggerStart  (std::function<void(const SOverlapData&)>   cb) { onTriggerStartCb   = std::move(cb); }
    void setOnTriggerStay   (std::function<void(const SOverlapData&)>   cb) { onTriggerStayCb    = std::move(cb); }
    void setOnTriggerEnd    (std::function<void(const SOverlapData&)>   cb) { onTriggerEndCb     = std::move(cb); }

    [[nodiscard]] ICapsuleCollisionBody* getPhysicsBody() const { return physicsBody; }

private:
    void createCollisionBody();
    void syncShape();
    void syncFieldsToBody();

private:
    IPhysicsEngineSubsystem* physicsEngine = nullptr;
    ICapsuleCollisionBody*   physicsBody   = nullptr;
    bool                     initialized   = false;

    std::function<void(const SCollisionData&)> onCollisionStartCb;
    std::function<void(const SCollisionData&)> onCollisionStayCb;
    std::function<void(const SCollisionData&)> onCollisionEndCb;
    std::function<void(const SOverlapData&)>   onTriggerStartCb;
    std::function<void(const SOverlapData&)>   onTriggerStayCb;
    std::function<void(const SOverlapData&)>   onTriggerEndCb;
};

#endif // CAPSULE_COLLISION_BODY_ENTITY_H