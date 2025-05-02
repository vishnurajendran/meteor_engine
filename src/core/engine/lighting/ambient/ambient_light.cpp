//
// Created by ssj5v on 24-01-2025.
//

#include "ambient_light.h"

#include "GL/glew.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/lighting/light_shader_constants.h"
#include "core/engine/lighting/lighting_system_manager.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/render_queue.h"

MAmbientLightEntity* MAmbientLightEntity::ambientLightInstance=nullptr;

MAmbientLightEntity::MAmbientLightEntity()
{
    if (ambientLightInstance != nullptr)
    {
        MERROR(STR("Meteor only support 1 ambient light node in scenes. "
                   "Please modify current ambient light entity, or delete and create again"));
        delete this;
        return;
    }

    lightType = ELightType::Ambient;
    ambientLightInstance = this;
    MLightSystemManager::getInstance()->registerLight(this);
}

void MAmbientLightEntity::setColor(const SColor& color)
{
    ambientLightData.color = SVector3(color.r, color.g, color.b);
}

SColor MAmbientLightEntity::getColor() const
{
    return SColor(ambientLightData.color.r, ambientLightData.color.g,ambientLightData.color.b, 1);
}

void MAmbientLightEntity::setIntensity(const float& intensity)
{
    ambientLightData.intensity = intensity;
}

float MAmbientLightEntity::getIntensity() const
{
    return ambientLightData.intensity;
}

void MAmbientLightEntity::onExit()
{
    MSpatialEntity::onExit();
    if (ambientLightInstance != this)
    {
        return;
    }

    MLightSystemManager::getInstance()->unregisterLight(this);
    ambientLightData.color = SVector3(0, 0, 0);
    ambientLightData.intensity = 0;

    // update color and intensity
    glBindBuffer(GL_UNIFORM_BUFFER, ambientLightDataBufferId);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ambientLightData), &ambientLightData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    ambientLightInstance = nullptr;
}
void MAmbientLightEntity::onDrawGizmo()
{
    auto texture = MAssetManager::getInstance()->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/arealight.png");
    MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f), texture->getTexture());
}

void MAmbientLightEntity::prepareLightRender()
{
    ambientLightData.enabled = getEnabled();
    if (!ambientLightDataBufferId)
    {
        glGenBuffers(1, &ambientLightDataBufferId);
        glBindBuffer(GL_UNIFORM_BUFFER, ambientLightDataBufferId);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(ambientLightData), nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBufferRange(GL_UNIFORM_BUFFER, LIGHT_INDEX_AMBIENT, ambientLightDataBufferId, 0, sizeof(ambientLightData));
    }

    //update color and intensity
    glBindBuffer(GL_UNIFORM_BUFFER, ambientLightDataBufferId);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ambientLightData), &ambientLightData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
