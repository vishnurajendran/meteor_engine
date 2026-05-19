//
// audio_waveform.cpp
//

#include "audio_waveform.h"
#include "miniaudio.h"

#include <algorithm>
#include <cmath>

MAudioWaveform::~MAudioWaveform()
{
    cancelled.store(true, std::memory_order_relaxed);
    if (worker.joinable())
        worker.join();
}

MAudioWaveform* MAudioWaveform::generate(const SString& filePath, int numBins)
{
    auto* wf = new MAudioWaveform();
    wf->worker = std::thread(&MAudioWaveform::decodeAndReduce, wf, filePath, numBins);
    return wf;
}

void MAudioWaveform::decodeAndReduce(const SString& filePath, int numBins)
{
    // channels = 0 means "use the file's native channel count."
    // This preserves stereo / multichannel layout for per-channel peaks.
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    ma_decoder decoder;

    if (ma_decoder_init_file(filePath.c_str(), &config, &decoder) != MA_SUCCESS)
    {
        failed.store(true, std::memory_order_release);
        ready.store(true, std::memory_order_release);
        return;
    }

    const ma_uint32 channels = decoder.outputChannels;
    ma_uint64 totalFrames = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);

    if (totalFrames == 0 || numBins <= 0 || channels == 0)
    {
        ma_decoder_uninit(&decoder);
        failed.store(true, std::memory_order_release);
        ready.store(true, std::memory_order_release);
        return;
    }

    channelCountResult = static_cast<int>(channels);
    numBinsResult      = numBins;

    // Allocate per-channel peak arrays.
    minPeaks.resize(channels);
    maxPeaks.resize(channels);
    for (ma_uint32 ch = 0; ch < channels; ++ch)
    {
        minPeaks[ch].resize(numBins, 0.f);
        maxPeaks[ch].resize(numBins, 0.f);
    }

    const ma_uint64 framesPerBin = totalFrames / static_cast<ma_uint64>(numBins);
    const ma_uint64 remainder    = totalFrames % static_cast<ma_uint64>(numBins);

    // Scratch buffer -- one bin's worth of interleaved samples.
    // Layout: [frame0_ch0, frame0_ch1, ..., frame1_ch0, frame1_ch1, ...]
    std::vector<float> buffer((framesPerBin + remainder) * channels);

    for (int bin = 0; bin < numBins; ++bin)
    {
        if (cancelled.load(std::memory_order_relaxed))
        {
            ma_decoder_uninit(&decoder);
            return;
        }

        // Last bin absorbs leftover frames from integer division.
        ma_uint64 framesToRead = framesPerBin;
        if (bin == numBins - 1)
            framesToRead += remainder;

        const ma_uint64 samplesToRead = framesToRead * channels;
        if (buffer.size() < samplesToRead)
            buffer.resize(samplesToRead);

        ma_uint64 framesRead = 0;
        ma_decoder_read_pcm_frames(&decoder, buffer.data(), framesToRead, &framesRead);

        // Per-channel min/max scan over the interleaved samples.
        for (ma_uint32 ch = 0; ch < channels; ++ch)
        {
            float lo = 0.f;
            float hi = 0.f;
            for (ma_uint64 f = 0; f < framesRead; ++f)
            {
                const float s = buffer[f * channels + ch];
                lo = std::min(lo, s);
                hi = std::max(hi, s);
            }
            minPeaks[ch][bin] = lo;
            maxPeaks[ch][bin] = hi;
        }
    }

    ma_decoder_uninit(&decoder);
    ready.store(true, std::memory_order_release);
}