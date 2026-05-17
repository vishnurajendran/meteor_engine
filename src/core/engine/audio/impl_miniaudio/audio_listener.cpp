//
// Created by ssj5v on 17-05-2026.
//

#include "audio_listener.h"
#include "core/utils/logger.h"
#include "miniaudio.h"

void MMiniAudioListener::init()
{
    if (engineHandle == nullptr)
    {
        MERROR("MiniAudioListener::Attempting to initialise listener without valid engine handle");
        return;
    }

    MLOG("MiniAudioListener::Created Audio Listener");
    initialized = true;
}

void MMiniAudioListener::cleanup()
{
    if (!initialized)
        return;

    MLOG("MiniAudioListener::Released Audio Listener");
}

void MMiniAudioListener::tick(const float& deltaTime)
{
    // no tick logic here for now.
}

void MMiniAudioListener::setPosition(const SVector3& pos)
{
    if (!initialized) return;
    ma_engine_listener_set_position(engineHandle, DEFAULT_LISTENER_INDEX, pos.x, pos.y, pos.z);
}

void MMiniAudioListener::setWorldUp(const SVector3& upVector)
{
    if (!initialized) return;
    ma_engine_listener_set_world_up(engineHandle, DEFAULT_LISTENER_INDEX, upVector.x, upVector.y, upVector.z);
}

void MMiniAudioListener::setDirection(const SVector3& direction)
{
    if (!initialized) return;
    ma_engine_listener_set_direction(engineHandle, DEFAULT_LISTENER_INDEX, direction.x, direction.y, direction.z);
}

void MMiniAudioListener::setVelocity(const SVector3& velocity)
{
    if (!initialized) return;
    ma_engine_listener_set_velocity(engineHandle, DEFAULT_LISTENER_INDEX, velocity.x, velocity.y, velocity.z);
}
