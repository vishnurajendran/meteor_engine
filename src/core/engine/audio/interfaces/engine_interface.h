//
// Created by ssj5v on 17-05-2026.
//

#ifndef ENGINE_INTERFACE_H
#define ENGINE_INTERFACE_H
#include "audiolistener_interface.h"
#include "audiosource_interface.h"
#include "core/engine/subsystem/subsystem_interface.h"

class IAudioEngineSubsystem : public IEngineSubSystem
{
public:
    ~IAudioEngineSubsystem() override = default;
    // factory methods
    virtual IAudioSource* createAudioSource() = 0;
    virtual bool releaseAudioSource(IAudioSource* source) = 0;

    virtual IAudioListener* createAudioListener() = 0;
    virtual bool releaseAudioListener(IAudioListener* listener) = 0;
};

#endif //ENGINE_INTERFACE_H
