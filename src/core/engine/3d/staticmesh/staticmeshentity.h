//
// Created by ssj5v on 05-10-2024.
//

#pragma once
#ifndef STATICMESHENTITY_H
#define STATICMESHENTITY_H

#include "core/graphics/core/render-pipeline/interfaces/drawable_interface.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/utils/aabb.h"

class MMaterialAsset;
class MMaterial;
class MStaticMeshAsset;

class MStaticMeshEntity : public MSpatialEntity, public IMeteorDrawable
{
public:
    MStaticMeshEntity();
    ~MStaticMeshEntity() override;

    // ---- IMeteorDrawable ----------------------------------------------------
    // Builds one SRenderItem per sub-mesh and submits them all to the collector.
    void submitRenderItem(IRenderItemCollector* collector) override;
    bool canDraw() override { return getEnabled(); }

    // ---- Entity lifecycle ---------------------------------------------------
    void onUpdate(float deltaTime)              override;
    void onDrawGizmo(SVector2 renderResolution) override;

    // ---- Asset setters ------------------------------------------------------
    void setStaticMeshAsset(MStaticMeshAsset* asset);
    void setMaterialAsset(MMaterialAsset* asset);
    void calculateBounds();

    // ---- Accessors ----------------------------------------------------------
    [[nodiscard]] MStaticMeshAsset* getStaticMeshAsset()   const { return staticMeshAsset; }
    [[nodiscard]] MMaterialAsset*   getMaterialAsset()      const { return materialAsset; }
    [[nodiscard]] MMaterial*        getMaterialInstance()   const { return materialInstance; }
    [[nodiscard]] AABB              getBounds()             const { return bounds; }

private:
    MStaticMeshAsset* staticMeshAsset   = nullptr;
    MMaterialAsset*   materialAsset     = nullptr;
    MMaterial*        materialInstance  = nullptr;
    AABB              bounds;
    SMatrix4          prevTransformMatrix;
};

#endif // STATICMESHENTITY_H