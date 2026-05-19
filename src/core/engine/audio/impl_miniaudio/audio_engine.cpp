//
// Created by ssj5v on 17-05-2026.
//

#include "audio_engine.h"

#include "audio_clip.h"
#include "audio_listener.h"
#include "audio_source.h"
#include "core/utils/logger.h"

void MMiniAudioEngineSubsystem::init()
{
    if (const ma_result result = ma_engine_init(nullptr, &engine); result != MA_SUCCESS)
    {
        MERROR("MiniAudioEngine::Failed initializing audio engine");
        return;
    }
    MLOG("MiniAudioEngine::Initialized audio engine");
    initialized = true;
}

void MMiniAudioEngineSubsystem::cleanup()
{
    if (!initialized)
        return;

    auto sourceCopy = std::vector(audioSources);
    for (auto* source : sourceCopy)
    {
        releaseAudioSource(source);
    }

    auto listenerCopy = std::vector(audioListeners);
    for (auto* listener : listenerCopy)
    {
        releaseAudioListener(listener);
    }

    auto clipCopy = std::vector(audioClips);
    for (auto* clip : clipCopy)
    {
        releaseAudioClip(clip);
    }

    audioSources.clear();
    audioListeners.clear();
    audioClips.clear();

    MLOG("MiniAudioEngine::Shutdown audio engine");
}

// -- Source --------------------------------------------------------------------

IAudioSource* MMiniAudioEngineSubsystem::createAudioSource()
{
    if (!initialized)
        return nullptr;

    auto* inst = new MMiniAudioSource();
    inst->internal_setEngineHandle(&engine);
    inst->init();
    audioSources.push_back(inst);
    return inst;
}

bool MMiniAudioEngineSubsystem::releaseAudioSource(IAudioSource* source)
{
    if (source == nullptr)
        return false;

    source->cleanup();
    std::erase(audioSources, source);
    delete source;
    return true;
}

// -- Listener -----------------------------------------------------------------

IAudioListener* MMiniAudioEngineSubsystem::createAudioListener()
{
    if (!initialized)
        return nullptr;

    auto* inst = new MMiniAudioListener();
    inst->internal_setEngineHandle(&engine);
    inst->internal_setListenerIndex(audioListeners.size());
    inst->init();
    audioListeners.push_back(inst);
    return inst;
}

bool MMiniAudioEngineSubsystem::releaseAudioListener(IAudioListener* listener)
{
    if (listener == nullptr)
        return false;

    listener->cleanup();
    std::erase(audioListeners, listener);
    delete listener;
    return true;
}

// -- Clip ---------------------------------------------------------------------

IAudioClip* MMiniAudioEngineSubsystem::createAudioClip(const SString& filePath)
{
    if (!initialized)
        return nullptr;

    auto* inst = new MMiniAudioClip();
    inst->internal_setEngineHandle(&engine);
    inst->internal_setFilePath(filePath);
    inst->init();
    audioClips.push_back(inst);
    return inst;
}

bool MMiniAudioEngineSubsystem::releaseAudioClip(IAudioClip* clip)
{
    if (clip == nullptr)
        return false;

    clip->cleanup();
    std::erase(audioClips, clip);
    delete clip;
    return true;
}