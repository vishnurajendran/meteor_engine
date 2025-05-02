//
// Created by ssj5v on 29-04-2025.
//

#include "dynamic_light.h"

#include "core/engine/lighting/lighting_system_manager.h"
void MDynamicLight::setColor(const SColor& color)
{
    lightData.color = {color.r, color.g, color.b};
}

SColor MDynamicLight::getColor() const
{
    return SColor(lightData.color.x,lightData.color.y, lightData.color.z,1) ;
}

void MDynamicLight::setIntensity(const float& intensity)
{
    lightData.intensity = intensity;
}

float MDynamicLight::getIntensity() const
{
    return lightData.intensity;
}

void MDynamicLight::setRange(const float& range)
{
    lightData.range = range;
}

void MDynamicLight::prepareLightRender()
{
    lightData.position = getWorldPosition();
    lightData.direction = getForwardVector();
}

SDynamicLightDataStruct MDynamicLight::getLightData() const
{
    return lightData;
}

void MDynamicLight::onUpdate(float deltaTime)
{
    MLightEntity::onUpdate(deltaTime);
    auto dist = glm::distance(getWorldPosition(), prevPosition);

    auto q1 = glm::normalize(getWorldRotation());
    auto q2 = glm::normalize(prevOrientation);
    float dotProduct = glm::dot(q1, q2);
    dotProduct = glm::clamp(dotProduct, -1.0f, 1.0f);
    float angle = 2.0f * acosf(fabs(dotProduct));

    if (dist >= EpsilonDist)
    {
        prevPosition = getWorldPosition();
        MLightSystemManager::getInstance()->requestLightSceneRebuild();
    }
    else if (angle >= EpsilonAngle)
    {
        prevOrientation = getWorldRotation();
        MLightSystemManager::getInstance()->requestLightSceneRebuild();
    }
}

