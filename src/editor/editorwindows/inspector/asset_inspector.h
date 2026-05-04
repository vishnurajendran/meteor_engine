// asset_inspector.h
// Suggested location: editor/editorwindows/assetinspector/
//
// Draws inspector UI for MAsset subclasses (MMaterialAsset, MTextureAsset, etc.)
// Called by MEditorInspectorWindow when Selected is an MAsset rather than
// an MSpatialEntity.

#pragma once
#ifndef ASSET_INSPECTOR_H
#define ASSET_INSPECTOR_H

#include <map>

#include "core/engine/assetmanagement/textasset/textasset.h"
#include "core/object/object.h"

class MAsset;
class MMaterialAsset;
class MTextureAsset;
class MMaterialPropertyControl;

class MAssetInspector
{
public:
    // Entry point — detects asset type and delegates.
    static void draw(MAsset* asset);

private:
    static void drawMaterialAsset(MMaterialAsset* asset);
    static void drawTextureAsset (MTextureAsset*  asset);
    static void drawTextAsset(MTextAsset* asset);
    static void drawGenericAsset (MAsset*         asset);

    // Per-asset-id property controls so texture reference dropdowns persist
    // between frames without being recreated on every draw.
    static std::map<SString, MMaterialPropertyControl*> propControlCache;
};

#endif