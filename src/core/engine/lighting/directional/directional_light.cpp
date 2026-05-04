//
// directional_light.cpp
//
#include "directional_light.h"
#include "../../../graphics/core/render-pipeline/stages/lighting/light_shader_constants.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/engine/texture/textureasset.h"

IMPLEMENT_CLASS(MDirectionalLight)

MDirectionalLight* MDirectionalLight::lightInstance = nullptr;

MDirectionalLight::MDirectionalLight()
{
    if (lightInstance != nullptr)
    {
        MERROR(STR("Meteor only supports 1 directional light node in scenes."));
        destroy();
        return;
    }
    lightType = ELightType::Directional;
    lightInstance = this;
    MLightSystemManager::getInstance()->registerLight(this);
}

// ── Color / intensity ─────────────────────────────────────────────────────────

void   MDirectionalLight::setColor(const SColor& c)     { color = SVector3(c.r, c.g, c.b); }
SColor MDirectionalLight::getColor() const
{
    const SVector3& v = color.get();
    return SColor(v.x, v.y, v.z, 1.0f);
}
void  MDirectionalLight::setIntensity(const float& i) { intensity = i; }
float MDirectionalLight::getIntensity() const          { return intensity.get(); }

// ── GPU upload ────────────────────────────────────────────────────────────────

void MDirectionalLight::prepareLightRender()
{
    lightData.lightColor     = color.get();
    lightData.lightIntensity = intensity.get();
    lightData.lightDirection = getForwardVector();
    lightData.enabled        = getEnabled();

    if (!bufferId)
    {
        glGenBuffers(1, &bufferId);
        glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(lightData), nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBufferRange(GL_UNIFORM_BUFFER, LIGHT_INDEX_DIRECTIONAL,
                          bufferId, 0, sizeof(lightData));
    }

    glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightData), &lightData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glFinish();
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void MDirectionalLight::onExit()
{
    MLightEntity::onExit();
    if (lightInstance != this) return;

    MLightSystemManager::getInstance()->unregisterLight(this);
    lightData.lightColor     = SVector3(0);
    lightData.lightIntensity = 0.0f;
    glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightData), &lightData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glFinish();
    lightInstance = nullptr;
}

void MDirectionalLight::onDrawGizmo(SVector2 renderResolution)
{
    auto texture = MAssetManager::getInstance()
        ->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/sun.png");
    MGizmos::drawRay(getWorldPosition(), -getForwardVector(), 0.5f, SColor(1, 1, 1, 1), 3);
    MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f), texture->getTexture());
}