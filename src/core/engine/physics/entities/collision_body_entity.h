//
// collision_body_entity.h
//

#ifndef COLLISION_BODY_ENTITY_H
#define COLLISION_BODY_ENTITY_H

#include <functional>

#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/physics/callback/physics_callback_reciever.h"
#include "core/engine/physics/data/collision_data.h"
#include "core/engine/physics/interface/bodies/collision_body_interface.h"
#include "core/engine/physics/interface/physics_engine_interface.h"

// MCollisionBodyEntity is the abstract base for all physics collision shapes.
// It owns every field and behaviour that is identical across box, sphere,
// cylinder, capsule, mesh, and convex hull bodies.
//
// Subclasses must implement:
//   createCollisionBody() - create the Jolt body and store it in a typed pointer
//   releaseBody()         - release the Jolt body and null the typed pointer
//   getBasePhysicsBody()  - return the typed pointer as ICollisionBody*
//
// Subclasses may override:
//   setupShapeCallbacks() - add field callbacks for shape-specific properties
//                           (radius, halfHeight, axis, meshAsset, etc.)
//                           Also used to replace the default bodyType onChange
//                           for shapes with constraints (e.g. mesh blocks Dynamic)
//   updateTransforms()    - call shape-specific sync (syncBounds, syncRadius...)
//   onDrawGizmo()         - draw the shape wireframe
//   onFixedUpdate()       - retry deferred body creation (mesh / convex hull)
class MCollisionBodyEntity : public MSpatialEntity, public IPhysicsCallbackReceiver
{
    DEFINE_ABSTRACT_SPATIAL_CLASS(MCollisionBodyEntity)

    // ---- Common serialisable fields ----------------------------------------
    // Every collision shape exposes these in the inspector.

    DECLARE_FIELD(bodyType,          ECollisionBodyType, ECollisionBodyType::StaticBody)
    DECLARE_FIELD(mass,              float,              10.0f)
    DECLARE_FIELD(affectedByGravity, bool,               false)
    DECLARE_FIELD(gravityScale,      float,              1.0f)
    DECLARE_FIELD(isSensor,          bool,               false)
    DECLARE_FIELD(linearDamping,     float,              0.0f)
    DECLARE_FIELD(angularDamping,    float,              0.0f)
    DECLARE_FIELD(restitution,       float,              0.3f)
    DECLARE_FIELD(friction,          float,              0.6f)
    // 0 = Default. Clamped to [0, 31] in the physicsLayer onChange callback.
    DECLARE_FIELD(physicsLayer,      int,                0)

public:
    MCollisionBodyEntity()           = default;
    ~MCollisionBodyEntity() override = default;

    // ---- MSpatialEntity lifecycle ------------------------------------------

    void onCreate()                override;
    void onStart()                 override;
    void onUpdate(float deltaTime) override;
    void onExit()                  override;

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

    // Return the subclass typed pointer as ICollisionBody*. Returns nullptr if
    // no body has been created yet.
    [[nodiscard]] virtual ICollisionBody* getBasePhysicsBody() const = 0;

protected:
    // ---- Pure virtuals -----------------------------------------------------

    // Create the Jolt body and store it in the subclass typed pointer.
    // Called from onStart and recreateBody. May complete without a body
    // (e.g. mesh/convex hull waiting for an asset) - that is valid.
    virtual void createCollisionBody() = 0;

    // Release the Jolt body via the engine factory and null the subclass pointer.
    virtual void releaseBody() = 0;

    // ---- Virtual with empty default ----------------------------------------

    // Called from onStart after common callbacks are set up. Override to wire
    // shape-specific field callbacks (radius onChange, halfHeight onChange, etc.)
    // and to replace the default bodyType onChange for shapes with constraints.
    virtual void setupShapeCallbacks() {}

    // ---- Helpers for subclasses --------------------------------------------

    // Must be called after createCollisionBody (or tryBuildBody) successfully
    // produces a body. Registers the body with the engine and syncs all common
    // fields. Subclasses that defer creation (mesh / convex hull) must call this
    // from their onFixedUpdate retry path after a body is created.
    void onBodyCreated();

    // Unregisters and releases the current body, then calls createCollisionBody
    // and onBodyCreated. Used by the default bodyType onChange and by subclasses
    // that rebuild on asset change (meshAsset onChange).
    void recreateBody();

    // ---- State accessible to subclasses ------------------------------------

    IPhysicsEngineSubsystem* physicsEngine = nullptr;
    bool                     initialized   = false;

private:
    // Syncs all common ICollisionBody fields to the current body.
    // Only called when getBasePhysicsBody() != nullptr.
    void syncFieldsToBody();

    // Wires onChange callbacks for all common fields. Also wires the default
    // bodyType onChange (release + recreate), which shapes with constraints
    // (e.g. mesh) can override in setupShapeCallbacks().
    void setupCommonCallbacks();

    std::function<void(const SCollisionData&)> onCollisionStartCb;
    std::function<void(const SCollisionData&)> onCollisionStayCb;
    std::function<void(const SCollisionData&)> onCollisionEndCb;
    std::function<void(const SOverlapData&)>   onTriggerStartCb;
    std::function<void(const SOverlapData&)>   onTriggerStayCb;
    std::function<void(const SOverlapData&)>   onTriggerEndCb;
};

#endif // COLLISION_BODY_ENTITY_H