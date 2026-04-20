//
// Created by ssj5v on 05-10-2024.
//

#pragma once
#ifndef STATICMESHENTITY_H
#define STATICMESHENTITY_H

#include "../../../graphics/core/render-pipeline/interfaces/drawable_interface.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/utils/aabb.h"


class MMaterialAsset;
class MStaticMeshDrawCall;
class MMaterial;
class MStaticMeshAsset;

class MStaticMeshEntity : public MSpatialEntity, public IMeteorDrawable {
public:
    void submitRenderItem(IRenderItemCollector* collector) override;
    bool canDraw() override { return getEnabled(); }
    void onDrawGizmo(SVector2 renderResolution) override;
    SString typeName() const override { return STR("static_mesh"); }
    void onExit() override;
    void onUpdate(float deltaTime) override;
private:
    MStaticMeshAsset* staticMeshAsset = nullptr;
    MMaterialAsset* materialAsset = nullptr;
    MMaterial* materialInstance = nullptr;
    MStaticMeshDrawCall* drawCall = nullptr;
    AABB bounds;
    bool castsShadow = true;

    SMatrix4  prevTransformMatrix;
public:
    MStaticMeshEntity();
    ~MStaticMeshEntity() override;
    void setStaticMeshAsset(MStaticMeshAsset* asset);
    void setMaterialAsset(MMaterialAsset* asset);
    void calculateBounds();
    [[nodiscard]] MStaticMeshAsset* getStaticMeshAsset() const { return staticMeshAsset; }
    [[nodiscard]] MMaterialAsset* getMaterialAsset() const { return materialAsset; }
    [[nodiscard]] MMaterial* getMaterialInstance() const { return materialInstance; }
    [[nodiscard]] AABB getBounds() const { return bounds; }

    [[nodiscard]] bool getCastsShadow() const { return castsShadow; }
    void setCastsShadow(bool v) { castsShadow = v; }
};

#endif // STATICMESHENTITY_H