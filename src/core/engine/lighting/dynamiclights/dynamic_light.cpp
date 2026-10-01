//
// Created by ssj5v on 29-04-2025.
//

#include "dynamic_light.h"

#include "../../../graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"

// Setters write through to both the serialized field and lightData — the field
// is what gets saved, lightData is what the renderer / shadow stage read.
void MDynamicLight::setColor(const SColor& c)
{
    color           = SVector3(c.r, c.g, c.b);
    lightData.color = color.get();
}

SColor MDynamicLight::getColor() const
{
    const SVector3& v = color.get();
    return SColor(v.x, v.y, v.z, 1);
}

void MDynamicLight::setIntensity(const float& i)
{
    intensity           = i;
    lightData.intensity = i;
}

float MDynamicLight::getIntensity() const
{
    return intensity.get();
}

void MDynamicLight::setRange(const float& r)
{
    range           = r;
    lightData.range = r;
}

void MDynamicLight::onDeserialise(const pugi::xml_node& node)
{
    MLightEntity::onDeserialise(node);

    lightData.color        = color.get();
    lightData.intensity    = intensity.get();
    lightData.range        = range.get();
    lightData.smoothShadow = smoothShadow.get() ? 1 : 0;

    // Range feeds the light BVH bounds — rebuild so culling uses the loaded value
    // instead of the default 10 the light was constructed with.
    MLightSystemManager::getInstance()->requestLightSceneRebuild();
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