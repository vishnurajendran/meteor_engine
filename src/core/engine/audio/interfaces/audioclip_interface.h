//
// audioclip_interface.h
//
// Runtime representation of a loaded audio clip. Provides metadata
// (length, channels, sample count) and a preload-vs-streaming toggle.
//
// The engine subsystem owns the pointer -- same lifecycle as
// IAudioSource and IAudioListener.
//

#ifndef AUDIOCLIP_INTERFACE_H
#define AUDIOCLIP_INTERFACE_H

#include "core/utils/sstring.h"
#include <cstdint>

class IAudioClip
{
public:
    virtual ~IAudioClip() = default;

    virtual void init()    = 0;
    virtual void cleanup() = 0;

    // -- Metadata (available after init) --------------------------------------

    virtual float       getLength()       const = 0;   // seconds
    virtual uint64_t    getNoOfSamples()  const = 0;   // total samples (frames * channels)
    virtual uint32_t    getNoOfChannels() const = 0;
    virtual uint32_t    getSampleRate()   const = 0;
    virtual uint64_t    getFrameCount()   const = 0;   // PCM frames

    // -- Load mode ------------------------------------------------------------
    // preloadAudio   -- sources that bind this clip decode the entire file
    //                   into memory up front (MA_SOUND_FLAG_DECODE).
    // enableStreaming -- default; sources stream from disk during playback.

    virtual void preloadAudio()    = 0;
    virtual void enableStreaming() = 0;
    virtual bool isPreloaded() const = 0;

    // -- Path -----------------------------------------------------------------

    virtual const SString& getFilePath() const = 0;
};

#endif // AUDIOCLIP_INTERFACE_H