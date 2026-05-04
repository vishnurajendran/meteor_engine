//
// ambient_light.cpp
//
#include "ambient_light.h"
#include "../../../graphics/core/render-pipeline/stages/lighting/light_shader_constants.h"
#include "GL/glew.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/engine/texture/textureasset.h"

IMPLEMENT_CLASS(MAmbientLightEntity)

MAmbientLightEntity* MAmbientLightEntity::ambientLightInstance = nullptr;

MAmbientLightEntity::MAmbientLightEntity()
{
    if (ambientLightInstance != nullptr)
    {
        MERROR(STR("Meteor only supports 1 ambient light node in scenes."));
        delete this;
        return;
    }
    lightType = ELightType::Ambient;
    ambientLightInstance = this;
    MLightSystemManager::getInstance()->registerLight(this);
}

// ── Color / intensity — fields are source of truth ───────────────────────────

void MAmbientLightEntity::setColor(const SColor& c)
{
    color = SVector3(c.r, c.g, c.b);
}

SColor MAmbientLightEntity::getColor() const
{
    const SVector3& v = color.get();
    return SColor(v.r, v.g, v.b, 1.0f);
}

void  MAmbientLightEntity::setIntensity(const float& i) { intensity = i; }
float MAmbientLightEntity::getIntensity() const          { return intensity.get(); }

// ── GPU upload ────────────────────────────────────────────────────────────────

void MAmbientLightEntity::prepareLightRender()
{
    // Sync field values → GPU struct before upload
    ambientLightData.color     = color.get();
    ambientLightData.intensity = intensity.get();
    ambientLightData.enabled   = getEnabled();

    if (!ambientLightDataBufferId)
    {
        glGenBuffers(1, &ambientLightDataBufferId);
        glBindBuffer(GL_UNIFORM_BUFFER, ambientLightDataBufferId);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(ambientLightData), nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBufferRange(GL_UNIFORM_BUFFER, LIGHT_INDEX_AMBIENT,
                          ambientLightDataBufferId, 0, sizeof(ambientLightData));
    }

    glBindBuffer(GL_UNIFORM_BUFFER, ambientLightDataBufferId);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ambientLightData), &ambientLightData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void MAmbientLightEntity::onExit()
{
    MSpatialEntity::onExit();
    if (ambientLightInstance != this) return;

    MLightSystemManager::getInstance()->unregisterLight(this);

    // Zero out GPU buffer before release
    ambientLightData.color     = SVector3(0);
    ambientLightData.intensity = 0;
    glBindBuffer(GL_UNIFORM_BUFFER, ambientLightDataBufferId);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ambientLightData), &ambientLightData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    ambientLightInstance = nullptr;
}

void MAmbientLightEntity::onDrawGizmo(SVector2 renderResolution)
{
    auto texture = MAssetManager::getInstance()
        ->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/arealight.png");
    MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f), texture->getTexture());
}