//
// MProceduralSkyboxEntity
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

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

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

    drawCall->setSunSize(0.04f);
    drawCall->setSunSizeConvergence(5.0f);
    drawCall->setAtmosphereThickness(1.0f);
    drawCall->setSkyTint(SVector3(0.5f, 0.5f, 0.5f));
    drawCall->setGroundColor(SVector3(0.22f, 0.20f, 0.18f));
    drawCall->setExposure(1.3f);

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

// ---------------------------------------------------------------------------
// IMeteorDrawable
// ---------------------------------------------------------------------------

void MProceduralSkyboxEntity::submitRenderItem(IRenderItemCollector*) {}

// ---------------------------------------------------------------------------
// Entity lifecycle
// ---------------------------------------------------------------------------

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

    // ---- Priority 1: directional light child drives sun direction ----------
    auto* dirLight = findDirectionalLightChild();
    if (dirLight)
    {
        // Light forward points toward the ground; sun direction is opposite.
        drawCall->setSunDirection(dirLight->getForwardVector());

        // Back-solve angles so the inspector sliders stay in sync.
        SVector3 dir = drawCall->getSunDirection();
        elevationDeg = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
        azimuthDeg   = glm::degrees(std::atan2(dir.x, dir.z));
    }
    else if (cycleSpeed != 0.0f)
    {
        // ---- Priority 2: day/night cycle animates elevation ----------------
        elevationDeg += cycleSpeed * 360.0f * deltaTime;
        if      (elevationDeg >  180.0f) elevationDeg -= 360.0f;
        else if (elevationDeg < -180.0f) elevationDeg += 360.0f;
        rebuildSunDirection();
    }

    // ---- Ambient light child: colour set from sky each frame ---------------
    auto* ambLight = findAmbientLightChild();
    if (ambLight)
        updateAmbientLight(ambLight);
}

void MProceduralSkyboxEntity::onDrawGizmo(SVector2 res)
{
    auto* tex = MAssetManager::getInstance()
        ->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/skybox.png");
    if (tex)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f),
                                 tex->getTexture());
}

// ---------------------------------------------------------------------------
// Child finders
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Ambient colour from sky
//
// Approximates the hemispherical average of the sky colour by blending
// between three reference colours keyed to sun elevation:
//
//   Night  (sin(elev) <= -0.15)  dark blue
//   Horizon(sin(elev) == 0)      warm orange / golden hour
//   Day    (sin(elev) >= 0.5)    sky blue
//
// The sky tint and exposure from the draw call are applied so the ambient
// automatically tracks any changes made in the inspector.
// ---------------------------------------------------------------------------

void MProceduralSkyboxEntity::updateAmbientLight(MAmbientLightEntity* light) const
{
    const float sinElev = std::sin(glm::radians(elevationDeg));

    // Reference colours — kept perceptually close to what the shader produces.
    const SVector3 nightColor  (0.02f, 0.04f, 0.12f);   // deep night blue
    const SVector3 horizonColor(0.75f, 0.45f, 0.18f);   // warm golden hour
    const SVector3 dayColor    (0.45f, 0.62f, 0.88f);   // sky blue

    SVector3 color;
    if (sinElev < 0.0f)
    {
        // Horizon → night: full night once sun is 15° below horizon.
        float t = glm::smoothstep(0.0f, 1.0f,
                      glm::clamp(-sinElev / 0.15f, 0.0f, 1.0f));
        color = glm::mix(horizonColor, nightColor, t);
    }
    else
    {
        // Horizon → full day: reaches day colour at ~30° elevation.
        float t = glm::smoothstep(0.0f, 1.0f,
                      glm::clamp(sinElev / 0.5f, 0.0f, 1.0f));
        color = glm::mix(horizonColor, dayColor, t);
    }

    // Apply the same sky tint the shader uses (neutral = 0.5 → ×1.0).
    if (drawCall)
        color *= drawCall->getSkyTint() * 2.0f;

    // Intensity: 0 at night, ramps up through dawn, full by mid-morning.
    float rawIntensity = glm::smoothstep(-0.15f, 0.3f, sinElev);
    float exposure     = drawCall ? drawCall->getExposure() : 1.3f;
    float intensity    = rawIntensity * exposure * 0.6f;

    light->setColor(SColor(color.r, color.g, color.b, 1.0f));
    light->setIntensity(intensity);
}

// ---------------------------------------------------------------------------
// Sun direction helpers
// ---------------------------------------------------------------------------

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
    const float elRad = glm::radians(elevationDeg);
    const float azRad = glm::radians(azimuthDeg);
    const float cosEl = std::cos(elRad);
    drawCall->setSunDirection(SVector3(cosEl * std::sin(azRad),
                                       std::sin(elRad),
                                       cosEl * std::cos(azRad)));
}

// ---------------------------------------------------------------------------
// Parameter pass-throughs
// ---------------------------------------------------------------------------

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