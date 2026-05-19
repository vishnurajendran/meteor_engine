//
// Created by ssj5v on 17-05-2026.
//

#ifndef AUDIO_SOURCE_H
#define AUDIO_SOURCE_H
#include "core/engine/audio/interfaces/audiosource_interface.h"
#include "core/object/object.h"
#include "miniaudio.h"

class MMiniAudioSource : public MObject, public IAudioSource
{
    DEFINE_OBJECT_SUBCLASS(MMiniAudioSource)
public:
    // init
    void init() override;
    void tick(const float& deltaTime) override;
    void cleanup() override;

    // internal
    void internal_setEngineHandle(ma_engine* internalEngineHandle) { engineHandle = internalEngineHandle; }

    // clip -- now takes an IAudioClip* instead of the raw asset handle
    void setClip(IAudioClip* clip) override;

    void play() override;
    void stop() override;
    void setLooping(const bool& looping) override;
    void setVolume(const float& volume) override;
    void setPitch(const float& pitch) override;

    // spatialization
    void setSpatializationEnabled(const bool& enabled) override;
    void setRollOff(const float& rollOff) override;
    void setMinDist(const float& minDist) override;
    void setMaxDist(const float& maxDist) override;
    void setDopplerStrength(const float& strength) override;
    void setPosition(const SVector3& position) override;
    void setDirection(const SVector3& direction) override;
    void setVelocity(const SVector3& velocity) override;

    // test
    void internal_testClip(SString clipPath);

private:
    ma_engine* engineHandle = nullptr;
    ma_sound soundHandle = {};
    bool initialized = false;
    IAudioClip* clip = nullptr;

    // sound flags
    ma_sound_flags soundFlags = MA_SOUND_FLAG_NO_SPATIALIZATION;
};

#endif //AUDIO_SOURCE_H