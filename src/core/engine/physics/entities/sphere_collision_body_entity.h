//
// Created by ssj5v on 22-05-2026.
//

#ifndef SPHERE_COLLISION_BODY_ENTITY_H
#define SPHERE_COLLISION_BODY_ENTITY_H

#include <functional>

#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/physics/callback/physics_callback_reciever.h"
#include "core/engine/physics/data/collision_data.h"
#include "core/engine/physics/interface/bodies/sphere/sphere_collision_body.h"
#include "core/engine/physics/interface/physics_engine_interface.h"

class MSphereCollisionBody : public MSpatialEntity, public IPhysicsCallbackReceiver
{
    DEFINE_SPATIAL_CLASS(MSphereCollisionBody)

    DECLARE_FIELD(bodyType,          ECollisionBodyType, ECollisionBodyType::StaticBody)
    DECLARE_FIELD(mass,              float,              10.0f)
    DECLARE_FIELD(affectedByGravity, bool,               false)
    DECLARE_FIELD(gravityScale,      float,              1.0f)
    DECLARE_FIELD(isSensor,          bool,               false)
    DECLARE_FIELD(linearDamping,     float,              0.0f)
    DECLARE_FIELD(angularDamping,    float,              0.0f)
    DECLARE_FIELD(restitution,       float,              0.3f)   // 0 = no bounce, 1 = fully elastic
    DECLARE_FIELD(friction,          float,              0.6f)
    DECLARE_FIELD(radius,            float,              1.0f)

public:
    MSphereCollisionBody()  = default;
    ~MSphereCollisionBody() override = default;

    void onCreate()                override;
    void onStart()                 override;
    void onUpdate(float deltaTime) override;
    void onExit()                  override;
    void onDrawGizmo(SVector2 res) override;
    void updateTransforms()        override;

    // ---- Scene creation helpers --------------------------------------------

    static MSphereCollisionBody* create(ECollisionBodyType type   = ECollisionBodyType::StaticBody,
                                        float              radius = 1.0f,
                                        float              mass   = 10.0f);

    static MSphereCollisionBody* createDynamic(float radius = 1.0f, float mass = 10.0f);
    static MSphereCollisionBody* createStatic (float radius = 1.0f);
    static MSphereCollisionBody* createTrigger(float radius = 1.0f);

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

    [[nodiscard]] ISphereCollisionBody* getPhysicsBody() const { return physicsBody; }

private:
    [[nodiscard]] SSphereBodySettings buildSettings() const;
    void syncFieldsToBody();
    void syncRadius();
    void createCollisionBody();

private:
    IPhysicsEngineSubsystem*  physicsEngine = nullptr;
    ISphereCollisionBody*     physicsBody   = nullptr;
    bool                      initialized   = false;

    std::function<void(const SCollisionData&)> onCollisionStartCb;
    std::function<void(const SCollisionData&)> onCollisionStayCb;
    std::function<void(const SCollisionData&)> onCollisionEndCb;
    std::function<void(const SOverlapData&)>   onTriggerStartCb;
    std::function<void(const SOverlapData&)>   onTriggerStayCb;
    std::function<void(const SOverlapData&)>   onTriggerEndCb;
};

#endif //SPHERE_COLLISION_BODY_ENTITY_H