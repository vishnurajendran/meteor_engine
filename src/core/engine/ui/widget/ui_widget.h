//
// ui_widget.h
//
// MUIWidget - a script-facing handle to a live UI widget.
//
//   self.hud = MUIWidget.create("assets/UI_Hud/Hud.muia")
//   self.hud:addToViewport(0)
//   self.hud:getBehaviour():setHealth(100)
//
// A handle is { viewport id, widget id, generation } and resolves on every
// call, so a widget destroyed elsewhere - or cleared when Play stops - gives
// isValid() == false and no-ops instead of crashing, the same contract as
// asset handles. Copies are cheap and all refer to the same widget.
//
// A widget is an element subtree with a behaviour: either a top-level widget
// added to the viewport (its root is an RmlUi document) or a nested one,
// written <widget src="guid:..." id="health"/> inside another widget's .rml.
//
// MUIWidget.create also takes an MAssetHandle, and getBehaviour() returns
// the behaviour's Lua table; both are bound by hand (ui_bindings.h).
//

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <cstdint>

#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/scripting/script_binding_macros.h"
#include "core/engine/ui/assets/ui_widget_asset.h"
#include "core/utils/sstring.h"

class MUIElement;
struct SUIWidgetInstance;

SCRIPT_BIND_CLASS()
class MUIWidget
{
public:
    MUIWidget() = default;
    MUIWidget(uint32_t viewportId, uint32_t widgetId, uint32_t generation)
        : viewportId(viewportId), widgetId(widgetId), generation(generation) {}

    // Creates a top-level widget in the main viewport, not yet added to it
    // (call addToViewport). Its behaviour's onCreate has run when this
    // returns. An invalid handle (with the error logged) on failure.
    SCRIPT_BIND_FUNC_STATIC()
    static MUIWidget create(TAssetHandle<MUIWidgetAsset> asset);

    // The same, from "guid:<id>" or an asset path.
    static MUIWidget createFromReference(const SString& reference);

    SCRIPT_BIND_FUNC()
    bool isValid() const;

    // Adds it to the viewport at zOrder (higher draws on top), or moves it to
    // zOrder if it is already there. Also re-adds a removed widget.
    // Top-level widgets only.
    SCRIPT_BIND_FUNC()
    void addToViewport(int zOrder);

    // Takes it off the screen. The widget and its behaviour's state are kept;
    // addToViewport shows it again.
    SCRIPT_BIND_FUNC()
    void removeFromViewport();

    // Removes it and frees it and its nested widgets. Every handle to it
    // becomes invalid. A nested widget is destroyed with its parent - destroy
    // the top-level widget instead.
    SCRIPT_BIND_FUNC()
    void destroy();

    SCRIPT_BIND_FUNC()
    bool isInViewport() const;

    SCRIPT_BIND_FUNC()
    int getZOrder() const;

    // Hides or shows it without removing it.
    SCRIPT_BIND_FUNC()
    void setVisible(bool visible);

    SCRIPT_BIND_FUNC()
    bool isVisible() const;

    // An element of this widget by its id. Only this widget's own elements
    // are searched, never those of its nested widgets, so ids only need to be
    // unique within one widget.
    SCRIPT_BIND_FUNC()
    MUIElement findElement(const SString& id) const;

    // A nested widget by its <widget id="...">, directly inside this one.
    SCRIPT_BIND_FUNC()
    MUIWidget findWidget(const SString& id) const;

    // The widget this one is nested in; invalid for top-level widgets.
    SCRIPT_BIND_FUNC()
    MUIWidget getParent() const;

    // Whether onWidgetTick runs while the game is paused. Defaults to the
    // .muia's setting. Input, events and animations run regardless.
    SCRIPT_BIND_FUNC()
    void setTickWhenPaused(bool tick);

    SCRIPT_BIND_FUNC()
    bool getTickWhenPaused() const;

    // The .muia's name ("Hud").
    SCRIPT_BIND_FUNC()
    SString getName() const;

    // -- Engine side -----------------------------------------------------------
    [[nodiscard]] SUIWidgetInstance* resolve() const;
    [[nodiscard]] uint32_t getViewportId() const { return viewportId; }
    [[nodiscard]] uint32_t getWidgetId()   const { return widgetId; }
    [[nodiscard]] uint32_t getGeneration() const { return generation; }
    [[nodiscard]] bool     isNull()        const { return generation == 0; }

    bool operator==(const MUIWidget& other) const
    {
        return viewportId == other.viewportId && widgetId == other.widgetId && generation == other.generation;
    }
    bool operator!=(const MUIWidget& other) const { return !(*this == other); }

private:
    uint32_t viewportId = 0;
    uint32_t widgetId   = 0;
    uint32_t generation = 0;   // 0 = null handle
};

// After the class: MUIElement holds an MUIWidget, and MUIWidget returns one.
#include "ui_element.h"

#endif // UI_WIDGET_H
