//
// ui_style_asset.h
//
// A UI stylesheet: an RmlUi .rcss file, held as text. Widgets apply their
// styles themselves (see MUIWidgetAsset); RmlUi reads the file through the
// file interface by its asset path.
//

#ifndef UI_STYLE_ASSET_H
#define UI_STYLE_ASSET_H

#include "core/engine/assetmanagement/asset/asset.h"

SCRIPT_BIND_CLASS()
class MUIStyleAsset : public MAsset
{
    DEFINE_OBJECT_SUBCLASS(MUIStyleAsset)
public:
    explicit MUIStyleAsset(const SString& path);

    SCRIPT_BIND_FUNC()
    SString getSource() const { return source; }

    bool requestReload() override;

private:
    bool load();
    SString source;
};

#endif // UI_STYLE_ASSET_H
