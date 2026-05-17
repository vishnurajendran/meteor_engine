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
class MMaterialPropertyControl;
class MAssetReferenceControl;

class MAssetInspector
{
public:
    static void draw(MAsset* asset);

private:
    static void drawMaterialAsset(MMaterialAsset* asset);
    static void drawTextureAsset (MTextureAsset*  asset);
    static void drawCubemapAsset (MCubemapAsset*  asset);
    static void drawTextAsset    (MTextAsset*     asset);
    static void drawGenericAsset (MAsset*         asset);

    static std::map<SString, MMaterialPropertyControl*> propControlCache;

    struct SCubemapFaceControls
    {
        std::array<MAssetReferenceControl*, 6> faces = {};
    };
    static std::map<SString, SCubemapFaceControls> cubemapFaceCache;
};

#endif // ASSET_INSPECTOR_H