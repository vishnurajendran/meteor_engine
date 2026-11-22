//
// ui_structure_asset.h
//
// A UI document's structure: an RmlUi .rml file, held as text. RmlUi itself
// reads it through the file interface (asset paths); this asset is what the
// engine knows about it - its <body> for nested widgets, and the widgets it
// nests (<widget src="...">) for hot reload and the inspector.
//
// Nested widget references should be "guid:<id>", like every other asset
// reference, so moving or renaming the widget keeps them intact. Paths still
// load; convertWidgetPathsToGuids() rewrites them (the inspector has a
// button for it).
//

#ifndef UI_STRUCTURE_ASSET_H
#define UI_STRUCTURE_ASSET_H

#include <vector>

#include "core/engine/assetmanagement/asset/asset.h"

SCRIPT_BIND_CLASS()
class MUIStructureAsset : public MAsset
{
    DEFINE_OBJECT_SUBCLASS(MUIStructureAsset)
public:
    explicit MUIStructureAsset(const SString& path);

    // The whole document source.
    SCRIPT_BIND_FUNC()
    SString getSource() const { return source; }

    // What goes inside a nested <widget>: the contents of <body>, or the whole
    // source if it has no <body>. The document's <head> (<link>, <style>) is
    // not part of it - a nested widget's styles come from its .muia.
    [[nodiscard]] SString getBodyContent() const;

    struct SWidgetReference
    {
        SString src;           // as written: "guid:..." or a path
        bool    isGuid = false;
        size_t  valueOffset = 0;   // where the src value starts in the source
        size_t  valueLength = 0;
    };

    // Every <widget src="..."> in the document, in source order.
    [[nodiscard]] const std::vector<SWidgetReference>& getWidgetReferences() const { return widgetReferences; }

    // Rewrites path references to "guid:<id>" and saves the file. Returns how
    // many were rewritten (0 when there was nothing to do), or -1 when the file
    // could not be saved. References that don't resolve are left alone.
    int convertWidgetPathsToGuids();

    bool requestReload() override;

private:
    bool load();
    void parseWidgetReferences();

    SString source;
    std::vector<SWidgetReference> widgetReferences;
};

#endif // UI_STRUCTURE_ASSET_H
