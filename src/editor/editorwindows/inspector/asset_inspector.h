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
class MShaderAsset;
class MStaticMeshAsset;
class MSkeletalMeshAsset;
class MAnimClipAsset;
class MFontAsset;
class MUIWidgetAsset;
class MUIStructureAsset;
class MUIStyleAsset;
struct SModelImportSettings;
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
    static void drawMaterialAsset   (MMaterialAsset*    asset);
    static void drawTextureAsset    (MTextureAsset*     asset);
    static void drawCubemapAsset    (MCubemapAsset*     asset);
    static void drawAudioClipAsset  (MAudioClipAsset*   asset);
    static void drawTextAsset       (MTextAsset*        asset);
    static void drawShaderAsset     (MShaderAsset*      asset);
    static void drawStaticMeshAsset (MStaticMeshAsset*  asset);
    static void drawSkeletalMeshAsset(MSkeletalMeshAsset* asset);   // asset_inspector_models.cpp
    static void drawAnimClipAsset   (MAnimClipAsset*    asset);     // asset_inspector_models.cpp
    static void drawFontAsset       (MFontAsset*        asset);     // asset_inspector_ui.cpp
    static void drawUIWidgetAsset   (MUIWidgetAsset*    asset);     // asset_inspector_ui.cpp
    static void drawUIStructureAsset(MUIStructureAsset* asset);     // asset_inspector_ui.cpp
    static void drawUIStyleAsset    (MUIStyleAsset*     asset);     // asset_inspector_ui.cpp

    // Import As / scale / pivot / max influences / sample rate, plus Save &
    // Reimport. Returns true if the asset was recreated (another importAs) -
    // the pointer is then stale and the caller must stop drawing it.
    static bool drawModelImportSettings(MAsset* asset, SModelImportSettings& settings);
    // "N entities still use this model as the old type - swap them?"
    static void drawEntitySwapPrompt();
    static SString pendingSwapAssetId;
    static void drawGenericAsset    (MAsset*            asset);

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