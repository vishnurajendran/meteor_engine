//
// Created by ssj5v on 05-10-2024.
//

#ifndef STATICMESHENTITY_H
#define STATICMESHENTITY_H
#include "core/engine/entities/spatial/spatial.h"
#include "core/graphics/core/drawable_interface.h"
#include "core/utils/aabb.h"


class MStaticMeshDrawCall;
class MMaterial;
class MStaticMeshAsset;

class MStaticMeshEntity : public MSpatialEntity, public IMeteorDrawable {
public:
    void prepareForDraw() override;
    void raiseDrawCall() override;
    bool canDraw() override { return getEnabled();}
    void onDrawGizmo() override;
    void onUpdate(float deltaTime) override;
private:
    MStaticMeshAsset* staticMeshAsset = nullptr;
    MMaterial* materialInstance = nullptr;
    MStaticMeshDrawCall* drawCall = nullptr;
    AABB bounds;

    SQuaternion prevRotation;
    SVector3 prevPosition;
public:
    MStaticMeshEntity();
    ~MStaticMeshEntity() override;
    void setStaticMeshAsset(MStaticMeshAsset* asset);
    void setMaterial(MMaterial *material);
    void calculateBounds();
    [[nodiscard]] AABB getBounds() const { return bounds; }
};

#endif //STATICMESHENTITY_H
