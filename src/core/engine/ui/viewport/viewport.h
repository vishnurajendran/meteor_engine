//
// viewport.h
//
// MViewport - one screen-space surface that UI widgets are added to: the game
// window in a player, the editor's Scene view while playing. Owns one
// Rml::Context, sized to the viewport's pixels, and the z-ordered list of
// top-level widgets added to it.
//
// Input reaches the viewport before gameplay. Each entry point returns true
// when the UI consumed the event; isConsumingMouse/Keyboard answer the same
// question at any time (SInput / SViewport / SCursor expose them to scripts).
//
// The host is whatever shows the viewport (the player window, the Scene
// view); the viewport asks it to apply cursor changes.
//

#ifndef UI_VIEWPORT_H
#define UI_VIEWPORT_H

#include <cstdint>
#include <vector>

#include "core/engine/input/cursor.h"
#include "core/engine/scripting/script_binding_macros.h"
#include "core/engine/ui/widget/ui_widget.h"
#include "core/engine/input/input.h"
#include "core/utils/glmhelper.h"
#include "core/utils/sstring.h"

namespace Rml
{
    class Context;
    class RenderInterface;
}
namespace sf { class Event; }

class IViewportHost
{
public:
    virtual ~IViewportHost() = default;

    // The cursor scripts asked for (SCursor). Locked hides the cursor
    // whatever `visible` says; the host keeps it at the viewport's centre and
    // tells the input subsystem (IInputSubsystem::setMouseLocked).
    virtual void applyCursor(bool visible, ECursorState state) = 0;

    // The cursor RmlUi asks for over an element ("pointer", "text", "move",
    // "" for the default). Optional.
    virtual void setCursorShape(const SString& /*cursorName*/) {}
};

// Scripts reach the main viewport with SViewport.getMainViewport():
//
//   local viewport = SViewport.getMainViewport()
//   viewport:addWidget(hud, 0)                 -- index = layer; higher draws on top
//   local menu = viewport:getCurrentWidget(10) -- the top widget on layer 10
//   viewport:clear()                           -- every widget off the screen
SCRIPT_BIND_CLASS()
class MViewport
{
public:
    MViewport(uint32_t id, IViewportHost* host);
    ~MViewport();

    MViewport(const MViewport&)            = delete;
    MViewport& operator=(const MViewport&) = delete;

    [[nodiscard]] uint32_t getId() const { return id; }

    // -- Script API --------------------------------------------------------------------
    // Shows a top-level widget on this viewport at layer `index` (higher draws
    // on top), or moves it there. The same as widget:addToViewport(index).
    SCRIPT_BIND_FUNC()
    void addWidget(const MUIWidget& widget, int index);

    // Takes it off the screen; it and its behaviour's state are kept.
    SCRIPT_BIND_FUNC()
    void removeWidget(const MUIWidget& widget);

    // The widget on layer `index`: the last one added there if there are
    // several. An invalid handle if the layer is empty.
    SCRIPT_BIND_FUNC()
    MUIWidget getCurrentWidget(int index) const;

    // Takes every widget off the screen. Like removeWidget, they aren't
    // destroyed: keep a handle to show one again, or destroy() it.
    SCRIPT_BIND_FUNC()
    void clear();

    SCRIPT_BIND_FUNC()
    int getWidth() const { return width; }

    SCRIPT_BIND_FUNC()
    int getHeight() const { return height; }

    // -- Host ------------------------------------------------------------------------
    void setHost(IViewportHost* newHost);
    [[nodiscard]] IViewportHost* getHost() const { return host; }

    // -- RmlUi context ---------------------------------------------------------------
    // Creates the context on first use. Needs RmlUi initialised (the subsystem
    // does that) and a GL context.
    bool ensureContext(Rml::RenderInterface* renderInterface);
    void destroyContext();
    [[nodiscard]] Rml::Context* getContext() const { return context; }

    // Pixel size. Resizes the context when it changes.
    void setSize(int width, int height);

    // Set when input or a script changed the UI after the frame's update, so
    // the renderer updates the context once more before drawing.
    void markNeedsUpdate() { needsUpdate = true; }
    bool consumeNeedsUpdate() { const bool value = needsUpdate; needsUpdate = false; return value; }

    // -- Input -----------------------------------------------------------------------
    // Coordinates are viewport pixels (0,0 top-left). Each returns true when
    // the UI consumed the event.
    bool processMouseMove(int x, int y);
    bool processMouseButton(int button, bool pressed);   // 0 left, 1 right, 2 middle
    bool processMouseWheel(float deltaX, float deltaY);  // positive Y = scroll up (SFML)
    void processMouseLeave();
    bool processKey(EKeyCode key, bool pressed);
    bool processText(const SString& utf8);
    // Releases buttons and keys held in the UI, e.g. when the window loses focus.
    void releaseAll();

    // An SFML window event, with window coordinates mapped into the viewport:
    // viewport = (window - origin) * scale. Returns true if consumed.
    bool processWindowEvent(const sf::Event& event, SVector2 origin = SVector2(0.0f), SVector2 scale = SVector2(1.0f));

    [[nodiscard]] bool isConsumingMouse() const;
    [[nodiscard]] bool isConsumingKeyboard() const;

    // -- Cursor ----------------------------------------------------------------------
    void setCursorVisible(bool visible);
    void setCursorState(ECursorState state);
    [[nodiscard]] bool         isCursorVisible() const { return cursorVisible; }
    [[nodiscard]] ECursorState getCursorState() const  { return cursorState; }
    // Visible and Free (when Play stops).
    void resetCursor();

    // -- Top-level widgets -------------------------------------------------------------
    // Kept sorted by (zOrder, addSequence): last draws on top.
    struct SWidgetEntry
    {
        uint32_t widgetId    = 0;
        int      zOrder      = 0;
        uint64_t addSequence = 0;
    };
    // Engine side of addWidget / removeWidget (MViewportSubsystem).
    void addWidgetEntry(uint32_t widgetId, int zOrder, uint64_t addSequence);
    void removeWidgetEntry(uint32_t widgetId);
    [[nodiscard]] const std::vector<SWidgetEntry>& getWidgets() const { return widgets; }
    [[nodiscard]] bool hasWidgets() const { return !widgets.empty(); }

private:
    [[nodiscard]] int currentModifiers() const;

    uint32_t       id      = 0;
    IViewportHost* host    = nullptr;
    Rml::Context*  context = nullptr;
    SString        contextName;

    int  width  = 1;
    int  height = 1;
    bool needsUpdate = false;

    void applyCursorToHost() const;

    bool         cursorVisible = true;
    ECursorState cursorState   = ECursorState::Free;

    bool mouseButtonsDown[3] = { false, false, false };

    std::vector<SWidgetEntry> widgets;
};

#endif // UI_VIEWPORT_H
