//
// Created by ssj5v on 17-05-2026.
//

#include "audioclip_asset.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/audio/interfaces/audioclip_interface.h"
#include "core/engine/audio/interfaces/engine_interface.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"

MAudioClipAsset::MAudioClipAsset(const SString& path) : MAsset(path)
{
    auto fileName = FileIO::getFileName(path);
    name = fileName;

    // The clip itself is created lazily; here we only confirm the file exists.
    valid = MAssetSources::getActive()->exists(path);
}

bool MAudioClipAsset::requestReload()
{
    releaseClip();
    valid = MAssetSources::getActive()->exists(path);
    return valid;
}

MAudioClipAsset::~MAudioClipAsset()
{
    releaseClip();
}

// -- Settings from .meta XML --------------------------------------------------
void MAudioClipAsset::loadSettings(const pugi::xml_document& metaData)
{
    auto node = metaData.child("audioclip");
    if (!node)
        return;

    auto preloadNode = node.child("preload");
    if (preloadNode)
        preloadSetting = SString(preloadNode.child_value()).toBool();
}

bool MAudioClipAsset::save()
{
    auto target = MAssetSources::getWritable();
    if (!target)
    {
        MERROR("MAudioClipAsset:: active asset source is read-only, cannot save " + path);
        return false;
    }

    // Load the existing meta (if any) so we don't clobber the GUID or data
    // other systems wrote into the same file.
    pugi::xml_document doc;
    target->readMeta(path, doc);

    // Find or create the <audioclip> node.
    auto node = doc.child("audioclip");
    if (!node)
        node = doc.append_child("audioclip");

    // Find or create the <preload> child.
    auto preloadNode = node.child("preload");
    if (!preloadNode)
        preloadNode = node.append_child("preload");

    preloadNode.text().set(preloadSetting ? "true" : "false");

    if (!target->writeMeta(path, doc))
    {
        MERROR("MAudioClipAsset:: Failed to write .meta file");
        return false;
    }
    return true;
}

// -- Lazy clip creation -------------------------------------------------------

IAudioClip* MAudioClipAsset::getAudioClip()
{
    if (audioClip)
        return audioClip;

    auto* engine = MEngineSubsystemRegistry::getSubsystem<IAudioEngineSubsystem>();
    if (!engine)
    {
        MERROR("MAudioClipAsset:: Audio engine subsystem not available");
        return nullptr;
    }

    audioClip    = engine->createAudioClip(getPath());
    cachedEngine = engine;

    if (!audioClip)
    {
        MERROR("MAudioClipAsset:: Failed to create audio clip");
        return nullptr;
    }

    // Apply the persisted preload setting.
    if (preloadSetting)
        audioClip->preloadAudio();
    else
        audioClip->enableStreaming();

    return audioClip;
}

void MAudioClipAsset::releaseClip()
{
    if (!audioClip)
        return;

    // Prefer the engine that created it. Fall back to the registry.
    IAudioEngineSubsystem* engine = cachedEngine;
    if (!engine)
        engine = MEngineSubsystemRegistry::getSubsystem<IAudioEngineSubsystem>();

    if (engine)
        engine->releaseAudioClip(audioClip);

    audioClip    = nullptr;
    cachedEngine = nullptr;
}

// -- Preload accessors --------------------------------------------------------

bool MAudioClipAsset::isPreloaded() const
{
    return preloadSetting;
}

void MAudioClipAsset::setPreloaded(bool preload)
{
    preloadSetting = preload;

    // Push to the live clip immediately if one exists.
    if (audioClip)
    {
        if (preload)
            audioClip->preloadAudio();
        else
            audioClip->enableStreaming();
    }
}