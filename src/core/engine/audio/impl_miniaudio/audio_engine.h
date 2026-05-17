//
// Created by ssj5v on 17-05-2026.
//

#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H
#include "core/engine/audio/interfaces/engine_interface.h"
#include "core/engine/subsystem/subsystem_interface.h"
#include "core/object/object.h"
#include "miniaudio.h"

class MMiniAudioEngineSubsystem : public MObject, public IAudioEngineSubsystem {
    DEFINE_OBJECT_SUBCLASS(MAudioEngine)
public:
    void init() override;
    void cleanup() override;

    [[nodiscard]] bool isInitialized() const { return initialized; }

    [[nodiscard]] IAudioSource* createAudioSource() override;
    [[nodiscard]] bool releaseAudioSource(IAudioSource* source) override;

    [[nodiscard]] IAudioListener* createAudioListener() override;
    [[nodiscard]] bool releaseAudioListener(IAudioListener* listener) override;

private:
    ma_engine engine = {};
    bool initialized = false;

    std::vector<IAudioSource*> audioSources;
    std::vector<IAudioListener*> audioListeners;
};



#endif //AUDIO_ENGINE_H
