//
// point_light.cpp
//
#include "point_light.h"

#include "../../../../graphics/core/render-pipeline/stages/lighting/dynamic_light_datatype.h"
#include "../../../../graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager_interface.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/textureasset.h"
#include "default_engine_icon_paths.h"

IMPLEMENT_SPATIAL_CLASS(MPointLight)

MPointLight::MPointLight()
{
    lightData.type = EDynamicLightDataType::Point;
    lightType      = ELightType::Point;
    if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
        renderer->getLightSystem().registerLight(this);
}

void MPointLight::onExit()
{
    MDynamicLight::onExit();
    if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
        renderer->getLightSystem().unregisterLight(this);
}

void MPointLight::onDrawGizmo(SVector2 renderResolution)
{
    auto icon = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
        ->getAsset<MTextureAsset>(SEngineAssetIconPaths::HIGHRES_TEX_GIZMOS_POINT_LIGHT);
    if (icon)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f), icon->getTexture());

    MGizmos::drawWireSphere(getWorldPosition(), lightData.range, SColor::skyBlue(), 0.1f);
}