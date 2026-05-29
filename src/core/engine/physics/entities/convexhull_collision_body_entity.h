//
// convexhull_collision_body_entity.h
//

#ifndef CONVEX_HULL_COLLISION_BODY_ENTITY_H
#define CONVEX_HULL_COLLISION_BODY_ENTITY_H

#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/physics/entities/collision_body_entity.h"
#include "core/engine/physics/interface/bodies/convexhull/convexhull_collsion_body.h"

// ConvexHullShape supports all body types including Dynamic — unlike MeshShape.
class MConvexHullCollisionBody : public MCollisionBodyEntity
{
    DEFINE_SPATIAL_CLASS(MConvexHullCollisionBody)

    DECLARE_FIELD(meshAsset, TAssetRef<MStaticMeshAsset>, {})

public:
    MConvexHullCollisionBody()  = default;
    ~MConvexHullCollisionBody() override = default;

    void onFixedUpdate(float fixedDeltaTime) override;
    void onDrawGizmo(SVector2 res)           override;

    [[nodiscard]] IConvexHullCollisionBody* getPhysicsBody() const { return physicsBody; }

protected:
    void createCollisionBody()                               override;
    void releaseBody()                                       override;
    [[nodiscard]] ICollisionBody* getBasePhysicsBody() const override { return physicsBody; }
    void setupShapeCallbacks()                               override;

private:
    void tryBuildBody(MStaticMeshAsset* asset);
    [[nodiscard]] MStaticMeshEntity* findMeshEntityChild() const;

    IConvexHullCollisionBody* physicsBody = nullptr;
};

#endif // CONVEX_HULL_COLLISION_BODY_ENTITY_H