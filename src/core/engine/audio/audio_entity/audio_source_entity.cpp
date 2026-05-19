//
// Created by ssj5v on 18-05-2026.
//

#include "audio_source_entity.h"

#include "SFML/Audio/Listener.hpp"
#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/audio/impl_miniaudio/audio_source.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/textureasset.h"
#include "default_engine_icon_paths.h"

IMPLEMENT_SPATIAL_CLASS(MAudioSource)

void MAudioSource::onCreate()
{
    MSpatialEntity::onCreate();
    engineSubsystem = MEngineSubsystemRegistry::getSubsystem<IAudioEngineSubsystem>();
    if (!engineSubsystem)
    {
        MERROR("MAudioSource:: AudioEngine subsystem not found");
        return;
    }
    source = engineSubsystem->createAudioSource();

    if (!source)
    {
        MERROR("MAudioSource:: Failed creating AudioSource");
        return;
    }

    loop.setOnChangeCallback([this](auto newVal)
    {
        if (source)
            source->setLooping(newVal);
    });

    volume.setOnChangeCallback([this](auto newVal)
    {
        if (source)
            source->setVolume(newVal);
    });

    useSpatial.setOnChangeCallback([this](auto newVal)
    {
        if (source)
            source->setSpatializationEnabled(newVal);
    });

    rollOff.setOnChangeCallback([this](auto newVal)
    {
        if (source)
            source->setRollOff(newVal);
    });

    minDist.setOnChangeCallback([this](auto newVal)
    {
        if (source)
            source->setMinDist(newVal);
    });

    maxDist.setOnChangeCallback([this](auto newVal)
    {
        if (source)
            source->setMaxDist(newVal);
    });

    dopplerStrength.setOnChangeCallback([this](auto newVal)
    {
       if (source)
           source->setDopplerStrength(newVal);
    });

    syncAudioEngineState();
    initialized = true;
    setCanTick(true);
}

void MAudioSource::onStart()
{
    MSpatialEntity::onStart();

    // Test code, Remove once we have assets ready to load
    auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    if (assetManager)
    {
        setClip(assetManager->getAsset<MAudioClipAsset>("assets/audio/test.mp3"));
        volume.set(0.1f);

        // enable spatialization
        useSpatial.set(true);
        play();
    }

}

void MAudioSource::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);

    if (!initialized)
        return;

    source->setDirection(getForwardVector());
    source->setPosition(getWorldPosition());

    const auto& currPos = getWorldPosition();
    const auto& velocity = (currPos - prevPos) / deltaTime;
    source->setVelocity(velocity);
    prevPos = currPos;

    source->tick(deltaTime);
}

void MAudioSource::onExit()
{
    MSpatialEntity::onExit();
    if (!initialized)
        return;

    engineSubsystem->releaseAudioSource(source);
    source = nullptr;
    engineSubsystem = nullptr;
}

void MAudioSource::onDrawGizmo(SVector2 res)
{
    const auto* assetPath = useSpatial.get() ? SEditorAssetPaths::HIGHRES_TEX_GIZMOS_AUDIO_SOURCE_3D
                                                       : SEditorAssetPaths::HIGHRES_TEX_GIZMOS_AUDIO_SOURCE_2D;

    const auto tex = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
        ->getAsset<MTextureAsset>(assetPath);
    if (tex)
        MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f), tex->getTexture());

    // Draw attenuation range spheres when spatialization is active
    if (useSpatial.get())
    {
        SVector3 pos = getWorldPosition();
        MGizmos::drawWireSphere(pos, minDist.get(), SColor(0.2f, 0.8f, 0.2f, 1.0f), 1.0f);
        MGizmos::drawWireSphere(pos, maxDist.get(), SColor(0.8f, 0.2f, 0.2f, 1.0f), 1.0f);
    }
}


void MAudioSource::setClip(TAssetHandle<MAudioClipAsset> clip)
{
    if (!initialized) return;
    if (!clip.isValid()) return;

    source->setClip(clip);
    outOfSync = true;
}

void MAudioSource::play()
{
    if (!initialized)
        return;

    if (outOfSync)
        syncAudioEngineState();

    source->play();
}

void MAudioSource::stop()
{
    if (!initialized)
        return;

    source->stop();
}

void MAudioSource::playOneShot(TAssetHandle<MAudioClipAsset> clip)
{
    if (!initialized)
        return;

    // set new clip, makes a new sound instance, so manually sync the states.
    source->setClip(clip);
    syncAudioEngineState();
    source->setLooping(false);
    source->play();

    // mark out of sync
    outOfSync = true;
}

void MAudioSource::syncAudioEngineState()
{
    source->setSpatializationEnabled(useSpatial.get());
    source->setDopplerStrength(dopplerStrength.get());
    source->setRollOff(rollOff.get());
    source->setMinDist(minDist.get());
    source->setMaxDist(maxDist.get());

    source->setVolume(volume.get());
    source->setPitch(pitch.get());
    source->setLooping(loop.get());

    outOfSync = false;
}