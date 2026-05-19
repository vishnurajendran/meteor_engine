//
// audio_waveform.h
//
// Asynchronous waveform peak generator for editor audio previews.
// Decodes the audio file on a background thread at its native channel
// count, reduces to min/max peaks per bin per channel, and exposes
// the result for ImGui rendering.
//
// Usage:
//   auto* wf = MAudioWaveform::generate("assets/audio/clip.mp3", 512);
//   // ... each frame ...
//   if (wf->isReady()) { /* read peaks per channel */ }
//   // ... when done ...
//   delete wf;   // joins the worker thread
//

#ifndef AUDIO_WAVEFORM_H
#define AUDIO_WAVEFORM_H

#include "core/utils/sstring.h"
#include <atomic>
#include <thread>
#include <vector>

class MAudioWaveform
{
public:
    ~MAudioWaveform();

    // Start async peak generation. Returns immediately.
    // Caller owns the returned pointer.
    static MAudioWaveform* generate(const SString& filePath, int numBins);

    // True once the worker thread has finished (peaks are safe to read).
    bool isReady() const { return ready.load(std::memory_order_acquire); }

    // True if the file could not be decoded or had no frames.
    bool hasFailed() const { return failed.load(std::memory_order_acquire); }

    // -- Per-channel peak data ------------------------------------------------
    // Outer vector: one entry per channel (size == getChannelCount()).
    // Inner vector: one entry per bin   (size == getBinCount()).
    // Only valid after isReady() returns true.

    const std::vector<std::vector<float>>& getMinPeaks() const { return minPeaks; }
    const std::vector<std::vector<float>>& getMaxPeaks() const { return maxPeaks; }

    int getBinCount()     const { return numBinsResult; }
    int getChannelCount() const { return channelCountResult; }

private:
    MAudioWaveform() = default;

    void decodeAndReduce(const SString& filePath, int numBins);

    // minPeaks[channel][bin], maxPeaks[channel][bin]
    std::vector<std::vector<float>> minPeaks;
    std::vector<std::vector<float>> maxPeaks;
    int numBinsResult      = 0;
    int channelCountResult = 0;

    std::atomic<bool> ready     { false };
    std::atomic<bool> failed    { false };
    std::atomic<bool> cancelled { false };

    std::thread worker;
};

#endif // AUDIO_WAVEFORM_H