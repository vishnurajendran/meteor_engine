//
// audio_clip.cpp
//

#include "audio_clip.h"
#include "core/utils/logger.h"

void MMiniAudioClip::init()
{
    if (filePath.empty())
    {
        MERROR("MMiniAudioClip:: No file path set");
        return;
    }

    // Open a temporary decoder just to read format metadata, then close it.
    // This is lightweight -- no PCM data is decoded here.
    ma_decoder_config config = ma_decoder_config_init_default();
    ma_decoder decoder;

    if (ma_decoder_init_file(filePath.c_str(), &config, &decoder) != MA_SUCCESS)
    {
        MERROR("MMiniAudioClip:: Failed to read metadata from file");
        return;
    }

    channelCount  = decoder.outputChannels;
    sampleRateHz  = decoder.outputSampleRate;

    ma_uint64 frames = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frames) == MA_SUCCESS)
    {
        totalFrames     = frames;
        lengthInSeconds = sampleRateHz > 0
                              ? static_cast<float>(frames) / static_cast<float>(sampleRateHz)
                              : 0.f;
    }

    ma_decoder_uninit(&decoder);

    initialized = true;
    MLOG("MMiniAudioClip:: Loaded clip metadata");
}

void MMiniAudioClip::cleanup()
{
    if (!initialized)
        return;

    initialized = false;
    MLOG("MMiniAudioClip:: Cleaned up");
}