//
// Created by ssj5v on 29-04-2025.
//

#include "point_light.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/lighting/dynamiclights/dynamic_light_datatype.h"
#include "core/engine/lighting/lighting_system_manager.h"
#include "core/engine/texture/textureasset.h"

MPointLight::MPointLight()
{
    lightData.type = EDynamicLightDataType::Point;
    lightType = ELightType::Point;
    MLightSystemManager::getInstance()->registerLight(this);
}

void MPointLight::onExit()
{
    MDynamicLight::onExit();
    MLightSystemManager::getInstance()->unregisterLight(this);
}

void MPointLight::onDrawGizmo()
{
    auto icon =
        MAssetManager::getInstance()->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/point_light.png");
    if (icon != nullptr)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f), icon->getTexture());

    MGizmos::drawWireSphere(getWorldPosition(), lightData.range, SColor::skyBlue(), 0.1f);
}

