//
// Created by ssj5v on 05-10-2024.
//

#include "staticmeshentity.h"

#include <cmath>

#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/render_queue.h"
#include "core/graphics/core/render-pipeline/stages/render_stage.h"
#include "staticmesh.h"
#include "staticmeshasset.h"

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

MStaticMeshEntity::MStaticMeshEntity() : MSpatialEntity()
{
    name                = "StaticMeshEntity";
    bounds              = { {0,0,0}, {0,0,0} };
    prevTransformMatrix = SMatrix4(0.0f); // != identity → triggers initial bounds calc
    MRenderQueue::addToSubmitables(this);
}

MStaticMeshEntity::~MStaticMeshEntity()
{
    // Always unregister from the render queue in the destructor — not just in
    // onExit() — so there is never a dangling pointer if onExit() was skipped
    // (e.g. force-deletes, editor reloads). Mirrors the pattern used by
    // MSkyboxEntity.
    MRenderQueue::removeFromSubmitables(this);

    if (materialInstance)
        delete materialInstance;
}

// ---------------------------------------------------------------------------
// IMeteorDrawable
// ---------------------------------------------------------------------------

void MStaticMeshEntity::submitRenderItem(IRenderItemCollector* collector)
{
    if (!staticMeshAsset) return;

    if (!materialInstance)
    {
        MERROR("MStaticMeshEntity::submitRenderItem — no material instance");
        return;
    }

    // Safety net: if the transform changed between onUpdate and submission
    // (e.g. an inspector edit), recalculate bounds before submitting.
    const SMatrix4 currentMatrix = getTransformMatrix();
    if (currentMatrix != prevTransformMatrix)
    {
        prevTransformMatrix = currentMatrix;
        calculateBounds();
    }

    // Submit one SRenderItem per sub-mesh.
    // Assumes each mesh exposes getVAO(), getEBO(), getIndexCount().
    // If a mesh has no EBO, set ebo = 0 and fill vertexCount instead.
    for (const auto& mesh : staticMeshAsset->getMeshes())
    {
        SRenderItem item;
        item.vao         = mesh->getVAO();
        item.ebo         = mesh->getEBO();
        item.indexCount  = mesh->getIndexCount();
        item.vertexCount = mesh->getVertexCount();  // used only when ebo == 0
        item.transform   = currentMatrix;
        item.material    = materialInstance;
        item.bounds      = bounds;
        item.castsShadow = castsShadow;
        item.sortOrder   = static_cast<int>(ERenderStageOrder::RS_Opaque);
        collector->submitRenderItem(item);
    }
}

// ---------------------------------------------------------------------------
// Entity lifecycle
// ---------------------------------------------------------------------------

void MStaticMeshEntity::onExit()
{
    MSpatialEntity::onExit();
    MRenderQueue::removeFromSubmitables(this);
}

void MStaticMeshEntity::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);

    const SMatrix4 currentMatrix = getTransformMatrix();
    if (currentMatrix != prevTransformMatrix)
    {
        prevTransformMatrix = currentMatrix;
        calculateBounds();
    }
}

void MStaticMeshEntity::onDrawGizmo(SVector2 renderResolution)
{
    MSpatialEntity::onDrawGizmo(renderResolution);
}

// ---------------------------------------------------------------------------
// Asset setters
// ---------------------------------------------------------------------------

void MStaticMeshEntity::setStaticMeshAsset(MStaticMeshAsset* asset)
{
    if (!asset)
    {
        MERROR("MStaticMeshEntity::setStaticMeshAsset — null argument");
        return;
    }
    staticMeshAsset = asset;
    calculateBounds();
}

void MStaticMeshEntity::setMaterialAsset(MMaterialAsset* asset)
{
    if (materialInstance)
        delete materialInstance;

    if (!asset)
    {
        MERROR("MStaticMeshEntity::setMaterialAsset — null argument");
        return;
    }

    materialAsset    = asset;
    materialInstance = asset->getInstance();
}

void MStaticMeshEntity::calculateBounds()
{
    if (!staticMeshAsset) return;

    SVector3 min( FLT_MAX);
    SVector3 max(-FLT_MAX);

    const SMatrix4 worldMatrix = getTransformMatrix();

    for (const auto& mesh : staticMeshAsset->getMeshes())
    {
        for (const auto& vertex : mesh->getVertices())
        {
            SVector3 worldVert = worldMatrix * SVector4(vertex.Position, 1.0f);
            min = glm::min(min, worldVert);
            max = glm::max(max, worldVert);
        }
    }

    bounds.min = min;
    bounds.max = max;
}