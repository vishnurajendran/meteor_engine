//
// Created by ssj5v on 26-01-2025.
//

#include "directional_light.h"

#include "core/engine/lighting/lighting_system_manager.h"

MDirectionalLight* MDirectionalLight::lightInstance = nullptr;

MDirectionalLight::MDirectionalLight()
{
    if (lightInstance != nullptr)
    {
        MERROR(STR("Meteor only support 1 directional light node in scenes. "
                  "Please modify current ambient light entity, or delete and create again"));
        delete this;
        return;
    }

    lightType = ELightType::Directional;
    lightInstance = this;
    MLightSystemManager::getInstance()->registerLight(this);
}

void MDirectionalLight::onExit()
{
    MLightEntity::onExit();
    if (lightInstance != this)
        return;

    MLightSystemManager::getInstance()->unregisterLight(this);
    // reset data
    lightData.lightColor = SVector3(0,0,0);
    lightData.lightIntensity = 0.0f;

    // update GPU buffer
    glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightData), &lightData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glFinish();

    //reset light instance
    lightInstance = nullptr;
}


void MDirectionalLight::setColor(const SColor& color)
{
    lightData.lightColor = SVector3(color.r, color.g, color.b);
}

SColor MDirectionalLight::getColor() const
{
    return SColor(lightData.lightColor.x,lightData.lightColor.y, lightData.lightColor.z, 1.0f);
}

void MDirectionalLight::setIntensity(const float& intensity)
{
    lightData.lightIntensity = intensity;
}

float MDirectionalLight::getIntensity() const
{
    return lightData.lightIntensity;
}

void MDirectionalLight::prepareLightRender()
{
    lightData.lightDirection = getForwardVector();

    if (!bufferId)
    {
        glGenBuffers(1, &bufferId);
        glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(lightData), nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBufferRange(GL_UNIFORM_BUFFER, LIGHT_INDEX_DIRECTIONAL, bufferId, 0, sizeof(lightData));
    }

    SString data = serializeVector(lightData.lightColor);
    //MLOG("Intensity: " + std::to_string(lightData.lightIntensity));
    //update directional light params.
    glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightData), &lightData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glFinish();
}


