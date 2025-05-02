//
// Created by ssj5v on 29-04-2025.
//

#include "spot_light.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/lighting/dynamiclights/dynamic_light_datatype.h"
#include "core/engine/lighting/lighting_system_manager.h"
#include "core/engine/texture/textureasset.h"

MSpotLight::MSpotLight()
{
    lightType = ELightType::Spot;
    lightData.type = EDynamicLightDataType::Spot;
    MLightSystemManager::getInstance()->registerLight(this);
}
void MSpotLight::onExit()
{
    MDynamicLight::onExit();
    MLightSystemManager::getInstance()->unregisterLight(this);
}

void MSpotLight::onDrawGizmo()
{
    drawSpotLightGizmo();
    auto icon =
        MAssetManager::getInstance()->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/spot_light.png");
    if (icon != nullptr)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f), icon->getTexture());
}

void MSpotLight::drawSpotLightGizmo()
{
    const int segments = 12;

    // Normalize forward vector
    SVector3 dir = -getForwardVector();

    // Find right and up vectors orthogonal to forward
    SVector3 right = normalize(glm::abs(dir.x) < 0.99f ? glm::cross(dir, SVector3(1, 0, 0)) : glm::cross(dir, SVector3(0, 1, 0)));
    SVector3 up = normalize(glm::cross(right, dir));

    // Compute cone base center and radius
    float halfAngleRad = lightData.angle * 0.5f;
    float coneRadius = lightData.range * tan(halfAngleRad);
    const auto position = getWorldPosition();
    SVector3 coneBaseCenter = position + dir * lightData.range;

    // Draw the central direction line
    MGizmos::drawLine(position, coneBaseCenter, SColor::skyBlue(), 0.15f);

    // Draw the base circle and cone lines
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = (i / (float)segments) * 2.0f * glm::pi<float>();
        float angle2 = ((i + 1) / (float)segments) * 2.0f * glm::pi<float>();

        SVector3 dir1 = right * std::cos(angle1) + up * std::sin(angle1);
        SVector3 dir2 = right * std::cos(angle2) + up * std::sin(angle2);

        SVector3 point1 = coneBaseCenter + dir1 * coneRadius;
        SVector3 point2 = coneBaseCenter + dir2 * coneRadius;

        // Base circle edge
        MGizmos::drawLine(point1, point2, SColor::skyBlue(), 0.15f);

        // Cone side line
        MGizmos::drawLine(position, point1, SColor::skyBlue(), 0.15f);
    }
}

float MSpotLight::getSpotAngle() const
{
    return glm::degrees(lightData.angle);
}

void MSpotLight::setSpotAngle(float angle)
{
    lightData.angle = glm::radians(angle);
}
