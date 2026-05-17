//
// Created by ssj5v on 17-05-2026.
//

#ifndef AUDIO_LISTENER_H
#define AUDIO_LISTENER_H
#include "miniaudio.h"
#include "core/engine/audio/interfaces/audiolistener_interface.h"
#include "core/object/object.h"

class MMiniAudioListener : public MObject, public IAudioListener {
    DEFINE_OBJECT_SUBCLASS(MMiniAudioListener)
public:
    void init() override;
    void tick(const float& deltaTime) override;
    void cleanup() override;

    // internal
    void internal_setEngineHandle(ma_engine* internalEngineHandle) { this->engineHandle = internalEngineHandle; }
    void internal_setListenerIndex(const int& index) { myListenerIndex = index; };

    // Spatialization
    void setPosition(const SVector3& pos) override;
    void setWorldUp(const SVector3& upVector) override;
    void setDirection(const SVector3& direction) override;
    void setVelocity(const SVector3& velocity) override;

private:
    ma_engine* engineHandle = nullptr;
    bool initialized = false;
    int myListenerIndex = DEFAULT_LISTENER_INDEX;

    static constexpr int DEFAULT_LISTENER_INDEX = 0;
};



#endif //AUDIO_LISTENER_H
