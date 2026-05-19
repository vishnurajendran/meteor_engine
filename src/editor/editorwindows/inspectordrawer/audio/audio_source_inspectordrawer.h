#pragma once
#ifndef AUDIO_SOURCE_INSPECTOR_H
#define AUDIO_SOURCE_INSPECTOR_H

#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/assetmanagement/asset/asset_ref_handle.h"
#include "core/engine/audio/asset/audioclip_asset.h"
#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"


class MAssetReferenceControl;

class MAudioSourceInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MAudioSourceInspectorDrawer)
public:
    SString getInspectorName() const override { return "Audio Source"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    MAudioSourceInspectorDrawer();

    MAssetReferenceControl*        clipAssetControl = nullptr;
    TAssetRef<MAudioClipAsset>     lastKnownClip;

    static bool registered;
};

#endif // AUDIO_SOURCE_INSPECTOR_H