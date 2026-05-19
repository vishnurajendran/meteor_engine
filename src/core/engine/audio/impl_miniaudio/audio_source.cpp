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

void MMiniAudioSource::setClip(IAudioClip* inClip)
{
    if (!initialized)
        return;

    if (!inClip)
        return;

    ma_sound_stop(&soundHandle);
    ma_sound_uninit(&soundHandle);

    this->clip = inClip;
    const auto* assetPath = clip->getFilePath().c_str();

    // If the clip is preloaded, tell miniaudio to decode the entire file
    // into memory up front. Otherwise stream from disk (flags = 0).
    const ma_uint32 flags = clip->isPreloaded() ? MA_SOUND_FLAG_DECODE : 0;

    ma_fence* fence       = nullptr;
    ma_sound_group* group = nullptr;
    const ma_result result = ma_sound_init_from_file(engineHandle, assetPath, flags, group, fence, &soundHandle);

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

void MMiniAudioSource::setRollOff(const float& rollOff)
{
    if (!initialized)
        return;

    ma_sound_set_rolloff(&soundHandle, rollOff);
}

void MMiniAudioSource::setMinDist(const float& minDist)
{
    if (!initialized)
        return;
    ma_sound_set_min_distance(&soundHandle, minDist);
}

void MMiniAudioSource::setMaxDist(const float& maxDist)
{
    if (!initialized)
        return;
    ma_sound_set_max_distance(&soundHandle, maxDist);
}

void MMiniAudioSource::setDopplerStrength(const float& strength)
{
    if (!initialized)
        return;

    ma_sound_set_doppler_factor(&soundHandle, std::clamp(strength, 0.0f, 1.0f));
}

void MMiniAudioSource::internal_testClip(SString clipPath)
{
    ma_sound_stop(&soundHandle);
    ma_sound_uninit(&soundHandle);
    ma_result result = ma_sound_init_from_file(
            engineHandle,
            clipPath.c_str(),
            0,
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

    ma_sound_set_volume(&soundHandle, std::clamp(volume, 0.0f, 1.0f));
}

void MMiniAudioSource::setPitch(const float& pitch)
{
    if (!initialized)
        return;
    ma_sound_set_pitch(&soundHandle, pitch);
}

void MMiniAudioSource::setSpatializationEnabled(const bool& enabled)
{
    if (!initialized)
        return;
    ma_sound_set_spatialization_enabled(&soundHandle, enabled);
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