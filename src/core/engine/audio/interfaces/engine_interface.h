//
// Created by ssj5v on 17-05-2026.
//

#ifndef ENGINE_INTERFACE_H
#define ENGINE_INTERFACE_H
#include "core/engine/subsystem/subsystem_interface.h"
#include "audioclip_interface.h"
#include "audiolistener_interface.h"
#include "audiosource_interface.h"

class IAudioEngineSubsystem : public IEngineSubSystem
{
public:
    ~IAudioEngineSubsystem() override = default;

    // -- Source factory --------------------------------------------------------
    virtual IAudioSource* createAudioSource() = 0;
    virtual bool releaseAudioSource(IAudioSource* source) = 0;

    // -- Listener factory -----------------------------------------------------
    virtual IAudioListener* createAudioListener() = 0;
    virtual bool releaseAudioListener(IAudioListener* listener) = 0;

    // -- Clip factory ---------------------------------------------------------
    virtual IAudioClip* createAudioClip(const SString& filePath) = 0;
    virtual bool releaseAudioClip(IAudioClip* clip) = 0;
};

#endif //ENGINE_INTERFACE_H