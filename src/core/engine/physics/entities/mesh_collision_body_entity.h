//
// mesh_collision_body_entity.h
//

#ifndef MESH_COLLISION_BODY_ENTITY_H
#define MESH_COLLISION_BODY_ENTITY_H

#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/physics/entities/collision_body_entity.h"
#include "core/engine/physics/interface/bodies/mesh/mesh_collision_body.h"

// MeshShape is Static/Kinematic only — Dynamic is blocked in setupShapeCallbacks.
// The body is built lazily: onStart tries immediately; onFixedUpdate retries each
// tick until a mesh asset is available.
class MMeshCollisionBody : public MCollisionBodyEntity
{
    DEFINE_SPATIAL_CLASS(MMeshCollisionBody)

    // Explicit asset override. Leave empty to auto-pick from the first
    // MStaticMeshEntity child.
    DECLARE_FIELD(meshAsset, TAssetRef<MStaticMeshAsset>, {})

public:
    MMeshCollisionBody()  = default;
    ~MMeshCollisionBody() override = default;

    void onFixedUpdate(float fixedDeltaTime) override;
    void onDrawGizmo(SVector2 res)           override;

    [[nodiscard]] IMeshCollisionBody* getPhysicsBody() const { return physicsBody; }

protected:
    void createCollisionBody()                               override;
    void releaseBody()                                       override;
    [[nodiscard]] ICollisionBody* getBasePhysicsBody() const override { return physicsBody; }
    void setupShapeCallbacks()                               override;

private:
    // Builds the Jolt body from an already-resolved asset. Stores result in
    // physicsBody but does NOT call onBodyCreated — the caller is responsible.
    void tryBuildBody(MStaticMeshAsset* asset);

    [[nodiscard]] MStaticMeshEntity* findMeshEntityChild() const;

    IMeshCollisionBody* physicsBody = nullptr;
};

#endif // MESH_COLLISION_BODY_ENTITY_H