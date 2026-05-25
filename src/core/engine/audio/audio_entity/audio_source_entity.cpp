//
// Created by ssj5v on 18-05-2026.
//

#include "audio_source_entity.h"

#include "SFML/Audio/Listener.hpp"
#include "core/application/application.h"
#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/audio/asset/audioclip_asset.h"
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

    clipRef.setOnChangeCallback([this](auto newVal)
    {
        setClip(newVal.getHandle());
    });

    syncAudioEngineState();
    initialized = true;
    setCanTick(true);
}

void MAudioSource::onStart()
{
    MSpatialEntity::onStart();

    // By onStart(), de-serialisation is complete and clipRef is populated.
    // Forward the stored reference to the audio backend now that we can.
    if (!clipRef.get().isEmpty())
        setClip(clipRef.getHandle());

    if (autoStart.get() && MApplication::getAppInstance()->isPlaying())
    {
        play();
        MLOG("MAudioSource:: Autoplaying");
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

    IAudioClip* audioClip = nullptr;

    if (clip.isNull())
    {
        clipRef.rawValue = TAssetRef<MAudioClipAsset>();
    }
    else
    {
        clipRef.rawValue = TAssetRef<MAudioClipAsset>(clip);
        MAudioClipAsset* asset = clip.get();
        if (asset)
            audioClip = asset->getAudioClip(); // getAudioClip() returns nullptr on failure itself
    }

    source->setClip(audioClip);
    outOfSync = true;
    MLOG("MAudioSource::updated clip reference");
}

void MAudioSource::play()
{
    if (!initialized)
        return;

    if (outOfSync)
        syncAudioEngineState();

    if (!clipRef.get().isValid())
    {
        MERROR("MAudioSource:: Clip reference is invalid");
        return;
    }

    source->play();
    sourcePlaying = true;
}

void MAudioSource::stop()
{
    if (!initialized)
        return;

    if (clipRef.get().isValid())
        source->stop();
    sourcePlaying = false;
}

void MAudioSource::playOneShot(TAssetHandle<MAudioClipAsset> clip)
{
    if (!initialized)
        return;

    if (!clip.isValid())
        return;

    MAudioClipAsset* asset = clip.get();
    if (!asset)
        return;

    clipRef.set(clip);
    IAudioClip* audioClip = asset->getAudioClip();
    if (!audioClip) return;

    source->setClip(audioClip);
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