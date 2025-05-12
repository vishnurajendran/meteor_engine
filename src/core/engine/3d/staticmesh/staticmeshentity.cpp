//
// Created by ssj5v on 05-10-2024.
//

#include "staticmeshentity.h"

#include <cmath>

#include "core/engine/3d/material/material.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/graphicsrenderer.h"
#include "core/graphics/core/render_queue.h"
#include "staticmesh.h"
#include "staticmeshasset.h"
#include "staticmeshdrawcall.h"

void MStaticMeshEntity::prepareForDraw()
{
    if(staticMeshAsset == nullptr) {
        MERROR("MStaticMeshEntity::raiseDrawCall(): No mesh asset");
        return;
    }

    if(materialInstance == nullptr) {
        MERROR("MStaticMeshEntity::raiseDrawCall(): No material instance");
        return;
    }

    if(drawCall == nullptr) {
        drawCall = new MStaticMeshDrawCall();
    }

    auto drawParams = SStaticMeshDrawParams();
    drawParams.materialInstance = materialInstance;
    drawParams.meshAssetRefference = staticMeshAsset;
    drawParams.modelMatrix = getTransformMatrix();
    drawParams.position = getWorldPosition();
    drawParams.meshBounds = bounds;
    drawCall->setParams(drawParams);
}

void MStaticMeshEntity::raiseDrawCall()
{
    if (drawCall == nullptr)
    {
        MWARN("MStaticMeshEntity::raiseDrawCall(): No draw call");
        return;
    }
    MGraphicsRenderer::submit(drawCall);
}
void MStaticMeshEntity::onDrawGizmo()
{
    MSpatialEntity::onDrawGizmo();
    /*const SVector3& min = bounds.min;
    const SVector3& max = bounds.max;

    // 8 corners of the box
    SVector3 c000 = {min.x, min.y, min.z};
    SVector3 c001 = {min.x, min.y, max.z};
    SVector3 c010 = {min.x, max.y, min.z};
    SVector3 c011 = {min.x, max.y, max.z};
    SVector3 c100 = {max.x, min.y, min.z};
    SVector3 c101 = {max.x, min.y, max.z};
    SVector3 c110 = {max.x, max.y, min.z};
    SVector3 c111 = {max.x, max.y, max.z};

    // Bottom face
    MGizmos::drawLine(c000, c100, SColor::yellow(), 0.15f, false);
    MGizmos::drawLine(c100, c101, SColor::yellow(), 0.15f, false);
    MGizmos::drawLine(c101, c001, SColor::yellow(), 0.15f, false);
    MGizmos::drawLine(c001, c000, SColor::yellow(), 0.15f, false);

    // Top face
    MGizmos::drawLine(c010, c110, SColor::yellow(), 0.15f, false);
    MGizmos::drawLine(c110, c111, SColor::yellow(), 0.15f, false);
    MGizmos::drawLine(c111, c011, SColor::yellow(), 0.15f, false);
    MGizmos::drawLine(c011, c010, SColor::yellow(), 0.15f, false);

    // Vertical edges
    MGizmos::drawLine(c000, c010, SColor::yellow(), 0.15f, false);
    MGizmos::drawLine(c100, c110, SColor::yellow(), 0.15f, false);
    MGizmos::drawLine(c101, c111, SColor::yellow(), 0.15f, false);
    MGizmos::drawLine(c001, c011, SColor::yellow(), 0.15f, false);*/
}

void MStaticMeshEntity::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);

    static constexpr float EpsilonAngle = 0.035f;
    static constexpr float EpsilonDistance = 0.001f;

    auto dist = glm::distance(getWorldPosition(), prevPosition);
    glm::quat currentRot = glm::normalize(getWorldRotation());
    glm::quat prevRot = glm::normalize(prevRotation);
    glm::quat delta = glm::normalize(currentRot * glm::inverse(prevRot));
    const float angle = glm::angle(delta);
    if (angle >= EpsilonAngle || dist >= EpsilonDistance) {
        prevRotation = currentRot;
        prevPosition = getWorldPosition();
        calculateBounds();
    }
}

MStaticMeshEntity::MStaticMeshEntity() : MSpatialEntity(){
    name = "StaticMeshEntity";
    bounds = {{0,0,0}, {0,0,0}};
    prevRotation = glm::identity<glm::quat>();
    prevPosition = SVector3(0);
    MRenderQueue::addToSubmitables(this);
}

MStaticMeshEntity::~MStaticMeshEntity() {
    if(materialInstance)
        delete materialInstance;
}

void MStaticMeshEntity::setStaticMeshAsset(MStaticMeshAsset *asset) {
    if(asset == nullptr)
        MERROR("MStaticMeshEntity::setStaticMeshAsset: null argument");

    this->staticMeshAsset = asset;
    calculateBounds();
}

void MStaticMeshEntity::setMaterial(MMaterial* material)
{
    if (material == nullptr)
        MERROR("MStaticMeshEntity::setMaterial: null argument");

    this->materialInstance = material;
}

void MStaticMeshEntity::calculateBounds()
{
    if (staticMeshAsset == nullptr)
        return;

    SVector3  min(FLT_MAX);
    SVector3  max(-FLT_MAX);

    for (const auto& mesh : staticMeshAsset->getMeshes())
    {
        for (const auto& vertex : mesh->getVertices())
        {
            SVector3 worldVert = getModelMatrix() * SVector4(vertex.Position, 1.0);
            min = glm::min(min, worldVert);
            max = glm::max(max, worldVert);
        }
    }

    bounds.min = min;
    bounds.max = max;
    //MLOG("Bounds calculated");
}
