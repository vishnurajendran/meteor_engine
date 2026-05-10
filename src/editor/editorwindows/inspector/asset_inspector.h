// asset_inspector.h
//
// Draws inspector UI for MAsset subclasses (MMaterialAsset, MTextureAsset, etc.)
// Called by MEditorInspectorWindow when Selected is an MAsset rather than
// an MSpatialEntity.

#pragma once
#ifndef ASSET_INSPECTOR_H
#define ASSET_INSPECTOR_H

#include <map>
#include <array>

#include "core/engine/assetmanagement/textasset/textasset.h"
#include "core/object/object.h"

class MAsset;
class MMaterialAsset;
class MTextureAsset;
class MCubemapAsset;
class MMaterialPropertyControl;
class MAssetReferenceControl;

class MAssetInspector : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MAssetInspector)
public:
    // Entry point — detects asset type and delegates.
    static void draw(MAsset* asset);

private:
    static void drawMaterialAsset(MMaterialAsset* asset);
    static void drawTextureAsset (MTextureAsset*  asset);
    static void drawCubemapAsset (MCubemapAsset*  asset);
    static void drawTextAsset    (MTextAsset*     asset);
    static void drawGenericAsset (MAsset*         asset);

    // Per-asset-id property controls so texture reference dropdowns persist
    // between frames without being recreated on every draw.
    static std::map<SString, MMaterialPropertyControl*> propControlCache;

    // Per-asset-id cubemap face reference controls (6 per cubemap asset).
    struct SCubemapFaceControls
    {
        std::array<MAssetReferenceControl*, 6> faces = {};
    };
    static std::map<SString, SCubemapFaceControls> cubemapFaceCache;
};

#endif