//
// procedural_sky.cpp
//
#include "procedural_sky.h"
#include "procedural_skybox_drawcall.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/lighting/ambient/ambient_light.h"
#include "core/engine/lighting/directional/directional_light.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "procedural_skybox_queue.h"

IMPLEMENT_CLASS(MProceduralSkyboxEntity)

// Construction / destruction
MProceduralSkyboxEntity::MProceduralSkyboxEntity()
{
    name = "ProceduralSkybox";
    auto* shaderAsset = MAssetManager::getInstance()
        ->getAsset<MShaderAsset>("meteor_assets/engine_assets/shaders/internal/proceduralskybox.mesl");
    if (!shaderAsset)
    {
        MERROR("MProceduralSkyboxEntity — failed to load procedural skybox shader");
        return;
    }

    drawCall = new MProceduralSkyboxDrawCall(shaderAsset->getShader());
    pushFieldsToDrawCall();     // apply default field values to the draw call
    rebuildSunDirection();
    MProceduralSkyboxQueue::add(drawCall);
    setCanTick(true);
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

// Serialization hook

void MProceduralSkyboxEntity::onDeserialise(const pugi::xml_node& node)
{
    MSpatialEntity::onDeserialise(node);    // loads all DECLARE_FIELDs
    if (drawCall)
    {
        pushFieldsToDrawCall();
        rebuildSunDirection();
    }
}

// Pushes all field values into the draw call.
// Called on construction and after deserialization.
void MProceduralSkyboxEntity::pushFieldsToDrawCall()
{
    if (!drawCall) return;
    drawCall->setSunSize(sunSize.get());
    drawCall->setSunSizeConvergence(sunSizeConvergence.get());
    drawCall->setAtmosphereThickness(atmosphereThickness.get());
    drawCall->setSkyTint(skyTint.get());
    drawCall->setGroundColor(groundColor.get());
    drawCall->setExposure(exposure.get());
}

// IMeteorDrawable

void MProceduralSkyboxEntity::submitRenderItem(IRenderItemCollector*) {}

// Lifecycle

void MProceduralSkyboxEntity::onExit()
{
    MSpatialEntity::onExit();
    if (drawCall)
        MProceduralSkyboxQueue::remove(drawCall);
}

void MProceduralSkyboxEntity::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);
    if (!drawCall) return;

    if (auto* dirLight = findDirectionalLightChild())
    {
        drawCall->setSunDirection(dirLight->getForwardVector());
        SVector3 dir = drawCall->getSunDirection();
        elevationDeg = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
        azimuthDeg   = glm::degrees(std::atan2(dir.x, dir.z));
    }
    else if (cycleSpeed.get() != 0.0f)
    {
        float elev = elevationDeg.get() + cycleSpeed.get() * 360.0f * deltaTime;
        if      (elev >  180.0f) elev -= 360.0f;
        else if (elev < -180.0f) elev += 360.0f;
        elevationDeg = elev;
        rebuildSunDirection();
    }

    if (auto* ambLight = findAmbientLightChild())
        updateAmbientLight(ambLight);
}

void MProceduralSkyboxEntity::onDrawGizmo(SVector2 res)
{
    auto* tex = MAssetManager::getInstance()
        ->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/skybox_procedural.png");
    if (tex)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f), tex->getTexture());
}

// Child finders
MDirectionalLight* MProceduralSkyboxEntity::findDirectionalLightChild() const
{
    for (auto* child : const_cast<MProceduralSkyboxEntity*>(this)->getChildren())
        if (auto* l = dynamic_cast<MDirectionalLight*>(child)) return l;
    return nullptr;
}

MAmbientLightEntity* MProceduralSkyboxEntity::findAmbientLightChild() const
{
    for (auto* child : const_cast<MProceduralSkyboxEntity*>(this)->getChildren())
        if (auto* l = dynamic_cast<MAmbientLightEntity*>(child)) return l;
    return nullptr;
}

// Ambient colour from sky

void MProceduralSkyboxEntity::updateAmbientLight(MAmbientLightEntity* light) const
{
    const float sinElev = std::sin(glm::radians(elevationDeg.get()));

    const SVector3 nightColor  (0.02f, 0.04f, 0.12f);
    const SVector3 horizonColor(0.75f, 0.45f, 0.18f);
    const SVector3 dayColor    (0.45f, 0.62f, 0.88f);

    SVector3 color;
    if (sinElev < 0.0f)
    {
        float t = glm::smoothstep(0.0f, 1.0f, glm::clamp(-sinElev / 0.15f, 0.0f, 1.0f));
        color   = glm::mix(horizonColor, nightColor, t);
    }
    else
    {
        float t = glm::smoothstep(0.0f, 1.0f, glm::clamp(sinElev / 0.5f, 0.0f, 1.0f));
        color   = glm::mix(horizonColor, dayColor, t);
    }

    if (drawCall)
        color *= drawCall->getSkyTint() * 2.0f;

    float rawIntensity = glm::smoothstep(-0.15f, 0.3f, sinElev);
    float exp          = drawCall ? drawCall->getExposure() : 1.3f;
    float finalIntensity = rawIntensity * exp * 0.6f;

    light->setColor(SColor(color.r, color.g, color.b, 1.0f));
    light->setIntensity(finalIntensity);
}

// Sun direction

void MProceduralSkyboxEntity::setSunAngles(float elev, float azim)
{
    elevationDeg = elev;
    azimuthDeg   = azim;
    rebuildSunDirection();
}

void MProceduralSkyboxEntity::setSunDirection(const SVector3& dir)
{
    if (drawCall) drawCall->setSunDirection(dir);
    SVector3 n   = glm::normalize(dir);
    elevationDeg = glm::degrees(std::asin(glm::clamp(n.y, -1.0f, 1.0f)));
    azimuthDeg   = glm::degrees(std::atan2(n.x, n.z));
}

SVector3 MProceduralSkyboxEntity::getSunDirection() const
{
    return drawCall ? drawCall->getSunDirection() : SVector3(0, 1, 0);
}

void MProceduralSkyboxEntity::rebuildSunDirection()
{
    if (!drawCall) return;
    const float elRad = glm::radians(elevationDeg.get());
    const float azRad = glm::radians(azimuthDeg.get());
    const float cosEl = std::cos(elRad);
    drawCall->setSunDirection(SVector3(cosEl * std::sin(azRad),
                                       std::sin(elRad),
                                       cosEl * std::cos(azRad)));
}

// Parameter pass-throughs — field is source of truth; drawCall is synced

void MProceduralSkyboxEntity::setSunSize(float v)            { sunSize = v;            if (drawCall) drawCall->setSunSize(v); }
void MProceduralSkyboxEntity::setSunSizeConvergence(float v) { sunSizeConvergence = v;  if (drawCall) drawCall->setSunSizeConvergence(v); }
void MProceduralSkyboxEntity::setAtmosphereThickness(float v){ atmosphereThickness = v; if (drawCall) drawCall->setAtmosphereThickness(v); }
void MProceduralSkyboxEntity::setSkyTint(const SVector3& v)  { skyTint = v;            if (drawCall) drawCall->setSkyTint(v); }
void MProceduralSkyboxEntity::setGroundColor(const SVector3& v){ groundColor = v;      if (drawCall) drawCall->setGroundColor(v); }
void MProceduralSkyboxEntity::setExposure(float v)           { exposure = v;           if (drawCall) drawCall->setExposure(v); }

float    MProceduralSkyboxEntity::getSunSize()             const { return sunSize.get(); }
float    MProceduralSkyboxEntity::getSunSizeConvergence()  const { return sunSizeConvergence.get(); }
float    MProceduralSkyboxEntity::getAtmosphereThickness() const { return atmosphereThickness.get(); }
SVector3 MProceduralSkyboxEntity::getSkyTint()             const { return skyTint.get(); }
SVector3 MProceduralSkyboxEntity::getGroundColor()         const { return groundColor.get(); }
float    MProceduralSkyboxEntity::getExposure()            const { return exposure.get(); }