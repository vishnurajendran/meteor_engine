//
// convex_hull_collision_body_entity.h
//

#ifndef CONVEX_HULL_COLLISION_BODY_ENTITY_H
#define CONVEX_HULL_COLLISION_BODY_ENTITY_H

#include <functional>

#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/physics/callback/physics_callback_reciever.h"
#include "core/engine/physics/data/collision_data.h"
#include "core/engine/physics/interface/bodies/convexhull/convexhull_collsion_body.h"
#include "core/engine/physics/interface/physics_engine_interface.h"

// ConvexHullShape supports all body types including Dynamic — unlike MeshShape.
// The convex hull is computed by Jolt from the vertex positions of the mesh;
// normals, UVs, and index order are ignored.
class MConvexHullCollisionBody : public MSpatialEntity, public IPhysicsCallbackReceiver
{
    DEFINE_SPATIAL_CLASS(MConvexHullCollisionBody)

    DECLARE_FIELD(bodyType,          ECollisionBodyType,          ECollisionBodyType::DynamicBody)
    DECLARE_FIELD(mass,              float,                       10.0f)
    DECLARE_FIELD(affectedByGravity, bool,                        true)
    DECLARE_FIELD(gravityScale,      float,                       1.0f)
    DECLARE_FIELD(isSensor,          bool,                        false)
    DECLARE_FIELD(linearDamping,     float,                       0.0f)
    DECLARE_FIELD(angularDamping,    float,                       0.0f)
    DECLARE_FIELD(restitution,       float,                       0.3f)
    DECLARE_FIELD(friction,          float,                       0.6f)
    DECLARE_FIELD(meshAsset,         TAssetRef<MStaticMeshAsset>, {})

public:
    MConvexHullCollisionBody()  = default;
    ~MConvexHullCollisionBody() override = default;

    void onCreate()                               override;
    void onStart()                                override;
    void onUpdate(float deltaTime)                override;
    void onFixedUpdate(float fixedDeltaTime)      override;
    void onExit()                                 override;
    void onDrawGizmo(SVector2 res)                override;

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

    [[nodiscard]] IConvexHullCollisionBody* getPhysicsBody() const { return physicsBody; }

private:
    void tryBuildBody(MStaticMeshAsset* asset);
    [[nodiscard]] MStaticMeshEntity* findMeshEntityChild() const;
    void setupFieldCallbacks();
    void syncFieldsToBody();

private:
    IPhysicsEngineSubsystem*  physicsEngine = nullptr;
    IConvexHullCollisionBody* physicsBody   = nullptr;
    bool                      initialized   = false;

    std::function<void(const SCollisionData&)> onCollisionStartCb;
    std::function<void(const SCollisionData&)> onCollisionStayCb;
    std::function<void(const SCollisionData&)> onCollisionEndCb;
    std::function<void(const SOverlapData&)>   onTriggerStartCb;
    std::function<void(const SOverlapData&)>   onTriggerStayCb;
    std::function<void(const SOverlapData&)>   onTriggerEndCb;
};

#endif // CONVEX_HULL_COLLISION_BODY_ENTITY_H