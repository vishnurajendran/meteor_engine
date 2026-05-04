//
// spot_light.cpp
//
#include "spot_light.h"
#include "../../../../graphics/core/render-pipeline/stages/lighting/dynamic_light_datatype.h"
#include "../../../../graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/texture/textureasset.h"

IMPLEMENT_CLASS(MSpotLight)

MSpotLight::MSpotLight()
{
    lightType      = ELightType::Spot;
    lightData.type = EDynamicLightDataType::Spot;
    MLightSystemManager::getInstance()->registerLight(this);
}

void MSpotLight::onExit()
{
    MDynamicLight::onExit();
    MLightSystemManager::getInstance()->unregisterLight(this);
}

float MSpotLight::getSpotAngle() const         { return glm::degrees(lightData.angle); }
void  MSpotLight::setSpotAngle(float angleDeg) { lightData.angle = glm::radians(angleDeg); }

void MSpotLight::onDrawGizmo(SVector2 renderResolution)
{
    drawSpotLightGizmo();
    auto icon = MAssetManager::getInstance()
        ->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/spot_light.png");
    if (icon)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f), icon->getTexture());
}

void MSpotLight::drawSpotLightGizmo()
{
    constexpr int segments = 12;
    SVector3 dir   = -getForwardVector();
    SVector3 right = normalize(glm::abs(dir.x) < 0.99f
                        ? glm::cross(dir, SVector3(1, 0, 0))
                        : glm::cross(dir, SVector3(0, 1, 0)));
    SVector3 up    = normalize(glm::cross(right, dir));

    float    halfAngleRad  = lightData.angle * 0.5f;
    float    coneRadius    = lightData.range * tan(halfAngleRad);
    SVector3 position      = getWorldPosition();
    SVector3 coneBaseCenter = position + dir * lightData.range;

    MGizmos::drawLine(position, coneBaseCenter, SColor::skyBlue(), 0.15f);

    for (int i = 0; i < segments; ++i)
    {
        float a1 = (i     / (float)segments) * 2.0f * glm::pi<float>();
        float a2 = ((i+1) / (float)segments) * 2.0f * glm::pi<float>();

        SVector3 d1 = right * std::cos(a1) + up * std::sin(a1);
        SVector3 d2 = right * std::cos(a2) + up * std::sin(a2);
        SVector3 p1 = coneBaseCenter + d1 * coneRadius;
        SVector3 p2 = coneBaseCenter + d2 * coneRadius;

        MGizmos::drawLine(p1, p2, SColor::skyBlue(), 0.15f);
        MGizmos::drawLine(position, p1, SColor::skyBlue(), 0.15f);
    }
}