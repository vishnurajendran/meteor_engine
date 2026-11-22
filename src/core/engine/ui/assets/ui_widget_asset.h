//
// ui_widget_asset.h
//
// A UI widget (.muia): the container that bundles a structure (.rml), one or
// more styles (.rcss, applied in order) and an optional behaviour (a Lua
// script extending WidgetBehaviour). Scripts create live widgets from it:
//
//   local hud = MUIWidget.create("assets/UI_Hud/Hud.muia")
//
// File format - references are GUIDs, like scenes, so moving or renaming the
// parts keeps the widget intact (paths are accepted when reading and
// rewritten as GUIDs on save):
//
//   <widget version="1" tickWhenPaused="true">
//       <structure>guid:...</structure>
//       <style>guid:...</style>          one or more
//       <behaviour>guid:...</behaviour>  optional
//   </widget>
//

#ifndef UI_WIDGET_ASSET_H
#define UI_WIDGET_ASSET_H

#include <vector>

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "ui_structure_asset.h"
#include "ui_style_asset.h"

SCRIPT_BIND_CLASS()
class MUIWidgetAsset : public MAsset
{
    DEFINE_OBJECT_SUBCLASS(MUIWidgetAsset)
public:
    static constexpr int FILE_VERSION = 1;

    explicit MUIWidgetAsset(const SString& path);

    // The .muia's file name without extension ("Hud"). Live widgets put the
    // class "w-<Name>" on their root, so generated styles can be scoped.
    SCRIPT_BIND_FUNC()
    SString getWidgetName() const;

    // Default for every instance; scripts change it per instance.
    SCRIPT_BIND_FUNC()
    bool getTickWhenPaused() const { return tickWhenPaused; }

    // -- Parts (resolved through the asset manager on every call) ---------------
    [[nodiscard]] TAssetHandle<MUIStructureAsset>          getStructure() const;
    [[nodiscard]] std::vector<TAssetHandle<MUIStyleAsset>> getStyles() const;
    [[nodiscard]] TAssetHandle<MAsset>                     getBehaviour() const;   // a script asset, or null

    // As stored: "guid:<id>" (or a path in a hand-written file).
    [[nodiscard]] const SString&              getStructureRef() const { return structureRef; }
    [[nodiscard]] const std::vector<SString>& getStyleRefs()    const { return styleRefs; }
    [[nodiscard]] const SString&              getBehaviourRef() const { return behaviourRef; }

    // Editing (inspector). Call save() to write the file.
    void setStructureRef(const SString& reference) { structureRef = reference; markDirty(); }
    void setStyleRefs(const std::vector<SString>& references) { styleRefs = references; markDirty(); }
    void setBehaviourRef(const SString& reference) { behaviourRef = reference; markDirty(); }
    void setTickWhenPaused(bool tick) { tickWhenPaused = tick; markDirty(); }

    bool save() override;
    bool requestReload() override;

    // The file contents for the given parts - also used to create new widgets.
    static SString serialise(const SString& structureRef, const std::vector<SString>& styleRefs,
                             const SString& behaviourRef, bool tickWhenPaused);

    // Dependencies: the structure, styles and behaviour.
    bool hasDeferredLoad() const override { return true; }
    bool dependsOn(const SString& assetPath) const override;
    // Checks that every part resolves (logged once per load).
    void deferredAssetLoad(bool forced) override;

private:
    bool load();

    SString              structureRef;
    std::vector<SString> styleRefs;
    SString              behaviourRef;
    bool                 tickWhenPaused = true;
};

#endif // UI_WIDGET_ASSET_H
