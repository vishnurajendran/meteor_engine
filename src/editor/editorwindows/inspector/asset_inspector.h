// asset_inspector.h

#pragma once
#ifndef ASSET_INSPECTOR_H
#define ASSET_INSPECTOR_H

#include <array>
#include <map>

#include "core/engine/assetmanagement/textasset/textasset.h"
#include "core/object/object.h"

class MAsset;
class MMaterialAsset;
class MTextureAsset;
class MCubemapAsset;
class MAudioClipAsset;
class MMaterialPropertyControl;
class MAssetReferenceControl;
class MAudioWaveform;
class IAudioSource;
class IAudioClip;
class IAudioEngineSubsystem;

class MAssetInspector
{
public:
    static void draw(MAsset* asset);

    // Call this to stop any in-progress audio preview.
    // Safe to call when nothing is playing.
    static void releaseAudioPreview();

private:
    static void drawMaterialAsset (MMaterialAsset*  asset);
    static void drawTextureAsset  (MTextureAsset*   asset);
    static void drawCubemapAsset  (MCubemapAsset*   asset);
    static void drawAudioClipAsset(MAudioClipAsset* asset);
    static void drawTextAsset     (MTextAsset*      asset);
    static void drawGenericAsset  (MAsset*          asset);

    // Renders the waveform peaks into an ImGui draw list region.
    static void drawWaveform(MAudioWaveform* waveform);

    static std::map<SString, MMaterialPropertyControl*> propControlCache;

    struct SCubemapFaceControls
    {
        std::array<MAssetReferenceControl*, 6> faces = {};
    };
    static std::map<SString, SCubemapFaceControls> cubemapFaceCache;

    // One waveform per audio asset, generated async on first view.
    static std::map<SString, MAudioWaveform*> waveformCache;

    // Holds the audio source used for in-editor clip preview.
    // Created on Play, released on Stop or when the inspected asset changes.
    struct SAudioPreviewState
    {
        IAudioSource*          source        = nullptr;
        IAudioClip*            clip          = nullptr;
        IAudioEngineSubsystem* engine        = nullptr;
        SString                activeAssetId;
        float                  volume        = 0.5f;
    };
    static SAudioPreviewState audioPreview;
};

#endif // ASSET_INSPECTOR_H