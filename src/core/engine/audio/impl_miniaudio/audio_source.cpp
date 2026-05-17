//
// Created by ssj5v on 17-05-2026.
//

#include "audio_source.h"
#include "core/utils/logger.h"

void MMiniAudioSource::init()
{
    if (engineHandle == nullptr)
    {
        MERROR("MMiniAudioSource:: Trying to initialise audio source without valid engine handle");
        return;
    }

    ma_result result = ma_sound_init_from_data_source(engineHandle, nullptr, MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, &soundHandle);
    if (result != MA_SUCCESS)
    {
        MERROR("MMiniAudioSource:: Failed to initialise audio source");
        return;
    }

    MLOG("MMiniAudioSource:: Initialized audio source");
    initialized = true;
}

void MMiniAudioSource::tick(const float& deltaTime)
{
    // no tick logic for now.
}

void MMiniAudioSource::cleanup()
{
    if (!initialized)
        return;

    ma_sound_uninit(&soundHandle);
    MLOG("MMiniAudioSource:: Cleaning up audio source");
}

void MMiniAudioSource::setClip(TAssetHandle<MAudioClipAsset> clip)
{
    if (!initialized)
        return;

    if (!clip.isValid())
        return;

    ma_sound_stop(&soundHandle);
    ma_sound_uninit(&soundHandle);

    this->clip = clip;
    ma_data_source* src = clip->getRawData();

    ma_result result =
        ma_sound_init_from_data_source(engineHandle, src, MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, &soundHandle);
    if (result != MA_SUCCESS)
    {
        MERROR("MMiniAudioSource:: Failed to set sound clip");
        return;
    }

    MLOG("MMiniAudioSource:: Successfully set clip");
}

void MMiniAudioSource::setLooping(const bool& looping)
{
    if (!initialized)
        return;

    ma_sound_set_looping(&soundHandle, looping);
}

void MMiniAudioSource::setPosition(const SVector3& position)
{
    if (!initialized)
        return;

    ma_sound_set_position(&soundHandle, position.x, position.y, position.z);
}

void MMiniAudioSource::setDirection(const SVector3& direction)
{
    if (!initialized)
        return;

    ma_sound_set_direction(&soundHandle, direction.x, direction.y, direction.z);
}

void MMiniAudioSource::setVelocity(const SVector3& velocity)
{
    if (!initialized)
        return;

    ma_sound_set_velocity(&soundHandle, velocity.x, velocity.y, velocity.z);
}

void MMiniAudioSource::internal_testClip(SString clipPath)
{
    ma_sound_stop(&soundHandle);
    ma_sound_uninit(&soundHandle);
    ma_result result = ma_sound_init_from_file(
            engineHandle,
            clipPath.c_str(),
            MA_SOUND_FLAG_NO_SPATIALIZATION,
            nullptr,
            nullptr,
            &soundHandle);

    if (result != MA_SUCCESS)
    {
        MERROR("MMiniAudioSource:: Failed to set sound clip");
        return;
    }

    MLOG("MMiniAudioSource:: Successfully set clip");
}


void MMiniAudioSource::setVolume(const float& volume)
{
    if (!initialized)
        return;

    ma_sound_set_volume(&soundHandle, std::clamp(volume, 0.0f,1.0f));
}

void MMiniAudioSource::play()
{
    if (!initialized)
        return;

    ma_sound_start(&soundHandle);
}

void MMiniAudioSource::stop()
{
    if (!initialized)
        return;

    ma_sound_stop(&soundHandle);
}