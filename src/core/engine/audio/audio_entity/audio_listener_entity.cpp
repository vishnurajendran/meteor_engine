//
// Created by ssj5v on 18-05-2026.
//

#include "audio_listener_entity.h"

#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/textureasset.h"
#include "default_engine_icon_paths.h"

IMPLEMENT_SPATIAL_CLASS(MAudioListener)

void MAudioListener::onCreate()
{
    MSpatialEntity::onCreate();
    engineSubsystem = MEngineSubsystemRegistry::getSubsystem<IAudioEngineSubsystem>();
    if (!engineSubsystem)
    {
        MERROR("MAudioListener:: AudioEngine subsystem not found");
        return;
    }
    listener = engineSubsystem->createAudioListener();

    if (!listener)
    {
        MERROR("MAudioListener:: Failed creating AudioListener");
        return;
    }

    listener->setWorldUp(SVector3(0, 1, 0));
    initialized = true;
    setCanTick(true);
}

void MAudioListener::onDrawGizmo(SVector2 res)
{
    const auto tex = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
        ->getAsset<MTextureAsset>(SEditorAssetPaths::HIGHRES_TEX_GIZMOS_AUDIO_LISTENER);
    if (tex)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f), tex->getTexture());
}


void MAudioListener::onExit()
{
    MSpatialEntity::onExit();

    if (!initialized)
        return;

    engineSubsystem->releaseAudioListener(listener);
}

void MAudioListener::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);

    if (!initialized)
        return;

    listener->setDirection(getForwardVector());
    listener->setPosition(getWorldPosition());
    const auto& velocity = (getWorldPosition() - lastPosition)/deltaTime;
    listener->setVelocity(velocity);
    listener->tick(deltaTime);

    lastPosition = getWorldPosition();
}
