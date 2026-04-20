//
// MProceduralSkyboxEntity
//

#include "procedural_sky.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "procedural_skybox_drawcall.h"
#include "procedural_skybox_queue.h"

MProceduralSkyboxEntity::MProceduralSkyboxEntity()
{
    name = "ProceduralSkybox";

    auto* shaderAsset = MAssetManager::getInstance()
        ->getAsset<MShaderAsset>(
            "meteor_assets/engine_assets/shaders/proceduralskybox.mesl");

    if (!shaderAsset)
    {
        MERROR("MProceduralSkyboxEntity — failed to load procedural skybox shader");
        return;
    }

    drawCall = new MProceduralSkyboxDrawCall(shaderAsset->getShader());

    // ---- Daytime defaults --------------------------------------------------
    elevationDeg = 55.0f;
    azimuthDeg   = 30.0f;
    drawCall->setSunSize(0.04f);
    drawCall->setSunSizeConvergence(5.0f);
    drawCall->setAtmosphereThickness(1.0f);
    drawCall->setSkyTint(SVector3(0.5f, 0.5f, 0.5f));
    drawCall->setGroundColor(SVector3(0.22f, 0.20f, 0.18f));
    drawCall->setExposure(1.3f);

    rebuildSunDirection();
    MProceduralSkyboxQueue::add(drawCall);
}

MProceduralSkyboxEntity::~MProceduralSkyboxEntity()
{
    if (drawCall)
    {
        MProceduralSkyboxQueue::remove(drawCall);
        delete drawCall;
        drawCall = nullptr;
    }
}

void MProceduralSkyboxEntity::submitRenderItem(IRenderItemCollector* /*collector*/) {}

void MProceduralSkyboxEntity::onExit()
{
    MSpatialEntity::onExit();
    if (drawCall)
        MProceduralSkyboxQueue::remove(drawCall);
}

void MProceduralSkyboxEntity::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);

    if (cycleSpeed != 0.0f)
    {
        elevationDeg += cycleSpeed * deltaTime;
        if      (elevationDeg >  180.0f) elevationDeg -= 360.0f;
        else if (elevationDeg < -180.0f) elevationDeg += 360.0f;
        rebuildSunDirection();
    }
}

void MProceduralSkyboxEntity::onDrawGizmo(SVector2 res)
{
    auto* tex = MAssetManager::getInstance()
        ->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/skybox.png");
    if (tex)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f),
                                 tex->getTexture());
}

void MProceduralSkyboxEntity::setSunAngles(float elev, float azim)
{
    elevationDeg = elev;
    azimuthDeg   = azim;
    rebuildSunDirection();
}

void MProceduralSkyboxEntity::setSunDirection(const SVector3& dir)
{
    if (drawCall) drawCall->setSunDirection(dir);
    elevationDeg = glm::degrees(std::asin(glm::normalize(dir).y));
}

SVector3 MProceduralSkyboxEntity::getSunDirection() const
{
    return drawCall ? drawCall->getSunDirection() : SVector3(0, 1, 0);
}

void MProceduralSkyboxEntity::rebuildSunDirection()
{
    if (!drawCall) return;
    const float elRad = glm::radians(elevationDeg);
    const float azRad = glm::radians(azimuthDeg);
    const float cosEl = std::cos(elRad);
    drawCall->setSunDirection(SVector3(cosEl * std::sin(azRad),
                                       std::sin(elRad),
                                       cosEl * std::cos(azRad)));
}

void  MProceduralSkyboxEntity::setSunSize           (float v)           { if (drawCall) drawCall->setSunSize(v); }
void  MProceduralSkyboxEntity::setSunSizeConvergence(float v)           { if (drawCall) drawCall->setSunSizeConvergence(v); }
void  MProceduralSkyboxEntity::setAtmosphereThickness(float v)          { if (drawCall) drawCall->setAtmosphereThickness(v); }
void  MProceduralSkyboxEntity::setSkyTint           (const SVector3& v) { if (drawCall) drawCall->setSkyTint(v); }
void  MProceduralSkyboxEntity::setGroundColor       (const SVector3& v) { if (drawCall) drawCall->setGroundColor(v); }
void  MProceduralSkyboxEntity::setExposure          (float v)           { if (drawCall) drawCall->setExposure(v); }

float    MProceduralSkyboxEntity::getSunSize()             const { return drawCall ? drawCall->getSunSize()             : 0.04f; }
float    MProceduralSkyboxEntity::getSunSizeConvergence()  const { return drawCall ? drawCall->getSunSizeConvergence()  : 5.0f; }
float    MProceduralSkyboxEntity::getAtmosphereThickness() const { return drawCall ? drawCall->getAtmosphereThickness() : 1.0f; }
SVector3 MProceduralSkyboxEntity::getSkyTint()             const { return drawCall ? drawCall->getSkyTint()             : SVector3(0.5f); }
SVector3 MProceduralSkyboxEntity::getGroundColor()         const { return drawCall ? drawCall->getGroundColor()         : SVector3(0.22f, 0.20f, 0.18f); }
float    MProceduralSkyboxEntity::getExposure()            const { return drawCall ? drawCall->getExposure()            : 1.3f; }