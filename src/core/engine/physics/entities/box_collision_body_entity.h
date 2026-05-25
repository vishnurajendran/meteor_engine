//
// Created by ssj5v on 22-05-2026.
//

#ifndef BOX_COLLISION_BODY_ENTITY_H
#define BOX_COLLISION_BODY_ENTITY_H

#include <functional>

#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/physics/callback/physics_callback_reciever.h"
#include "core/engine/physics/data/collision_data.h"
#include "core/engine/physics/interface/bodies/box/box_collision_body.h"
#include "core/engine/physics/interface/physics_engine_interface.h"

class MBoxCollisionBody : public MSpatialEntity, public IPhysicsCallbackReceiver
{
    DEFINE_SPATIAL_CLASS(MBoxCollisionBody)

    DECLARE_FIELD(bodyType,          ECollisionBodyType, ECollisionBodyType::StaticBody)
    DECLARE_FIELD(mass,              float,              10.0f)
    DECLARE_FIELD(affectedByGravity, bool,               false)
    DECLARE_FIELD(gravityScale,      float,              1.0f)
    DECLARE_FIELD(isSensor,          bool,               false)
    DECLARE_FIELD(linearDamping,     float,              0.0f)
    DECLARE_FIELD(angularDamping,    float,              0.0f)
    DECLARE_FIELD(restitution,       float,              0.3f)   // 0 = no bounce, 1 = fully elastic
    DECLARE_FIELD(friction,          float,              0.6f)
    DECLARE_FIELD(halfExtentX,       float,              0.5f)
    DECLARE_FIELD(halfExtentY,       float,              0.5f)
    DECLARE_FIELD(halfExtentZ,       float,              0.5f)

public:
    MBoxCollisionBody()  = default;
    ~MBoxCollisionBody() override = default;

    void onCreate()                override;
    void onStart()                 override;
    void onUpdate(float deltaTime) override;
    void onExit()                  override;
    void onDrawGizmo(SVector2 res) override;
    void updateTransforms()        override;

    // ---- Scene creation helpers --------------------------------------------

    static MBoxCollisionBody* create(ECollisionBodyType type        = ECollisionBodyType::StaticBody,
                                     SVector3           halfExtents = { 0.5f, 0.5f, 0.5f },
                                     float              mass        = 10.0f);

    static MBoxCollisionBody* createDynamic(SVector3 halfExtents = { 0.5f, 0.5f, 0.5f },
                                            float    mass        = 10.0f);

    static MBoxCollisionBody* createStatic(SVector3 halfExtents = { 0.5f, 0.5f, 0.5f });

    static MBoxCollisionBody* createTrigger(SVector3 halfExtents = { 0.5f, 0.5f, 0.5f });

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

    [[nodiscard]] IBoxCollisionBody* getPhysicsBody() const { return physicsBody; }

private:
    [[nodiscard]] SBoxPhysicsBodySettings buildSettings() const;
    void syncBounds();
    void syncFieldsToBody();
    void createCollisionBody();

private:
    IPhysicsEngineSubsystem*   physicsEngine = nullptr;
    IBoxCollisionBody*         physicsBody   = nullptr;
    bool                       initialized   = false;

    std::function<void(const SCollisionData&)> onCollisionStartCb;
    std::function<void(const SCollisionData&)> onCollisionStayCb;
    std::function<void(const SCollisionData&)> onCollisionEndCb;
    std::function<void(const SOverlapData&)>   onTriggerStartCb;
    std::function<void(const SOverlapData&)>   onTriggerStayCb;
    std::function<void(const SOverlapData&)>   onTriggerEndCb;
};

#endif //BOX_COLLISION_BODY_ENTITY_H