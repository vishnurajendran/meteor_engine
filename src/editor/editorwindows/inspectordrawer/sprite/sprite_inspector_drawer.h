#pragma once
#ifndef SPRITE_INSPECTOR_DRAWER_H
#define SPRITE_INSPECTOR_DRAWER_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MAssetReferenceControl;

class MSpriteInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MSpriteInspectorDrawer)
public:
    SString getInspectorName() const override { return "Sprite"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    MSpriteInspectorDrawer();

    MAssetReferenceControl* textureControl = nullptr;
    static bool registered;
};

#endif // SPRITE_INSPECTOR_DRAWER_H
