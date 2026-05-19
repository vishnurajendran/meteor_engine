//
// audio_clip.h
//
// Miniaudio-backed IAudioClip. Opens a temporary ma_decoder on init
// to read format metadata, then closes it -- no PCM data is held.
//
// preloadAudio() / enableStreaming() set a flag that IAudioSource
// implementations read when creating their ma_sound.
//

#ifndef AUDIO_CLIP_H
#define AUDIO_CLIP_H

#include "core/engine/audio/interfaces/audioclip_interface.h"
#include "core/object/object.h"
#include "miniaudio.h"

class MMiniAudioClip : public MObject, public IAudioClip
{
    DEFINE_OBJECT_SUBCLASS(MMiniAudioClip)
public:
    void init()    override;
    void cleanup() override;

    // Called by the engine factory before init().
    void internal_setEngineHandle(ma_engine* handle) { engineHandle = handle; }
    void internal_setFilePath(const SString& path)   { filePath = path; }

    // -- Metadata -------------------------------------------------------------

    float       getLength()       const override { return lengthInSeconds; }
    uint64_t    getNoOfSamples()  const override { return totalFrames * channelCount; }
    uint32_t    getNoOfChannels() const override { return channelCount; }
    uint32_t    getSampleRate()   const override { return sampleRateHz; }
    uint64_t    getFrameCount()   const override { return totalFrames; }

    // -- Load mode ------------------------------------------------------------

    void preloadAudio()        override { preloaded = true; }
    void enableStreaming()     override { preloaded = false; }
    bool isPreloaded()   const override { return preloaded; }

    // -- Path -----------------------------------------------------------------

    const SString& getFilePath() const override { return filePath; }

private:
    ma_engine* engineHandle   = nullptr;
    SString    filePath;
    bool       initialized    = false;

    // Cached metadata
    float      lengthInSeconds = 0.f;
    uint64_t   totalFrames     = 0;
    uint32_t   channelCount    = 0;
    uint32_t   sampleRateHz    = 0;

    bool       preloaded       = false;
};

#endif // AUDIO_CLIP_H