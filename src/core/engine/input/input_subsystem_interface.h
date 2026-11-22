//
// input_subsystem_interface.h
//
// Per-frame keyboard and mouse state, as an engine subsystem. The application
// updates it once per frame (poll(), or releaseAll() when its window has no
// focus) before anything reads input. Keys are read from the OS in poll();
// the mouse is fed from window events (onMouse*), already mapped to the game
// viewport, and latched by poll(). Gameplay code and scripts read it through the
// SInput facade; this interface is for the code that drives it.
//
//   auto* input = MEngineSubsystemRegistry::getSubsystem<IInputSubsystem>();
//

#ifndef INPUT_SUBSYSTEM_INTERFACE_H
#define INPUT_SUBSYSTEM_INTERFACE_H

#include "core/engine/input/input.h"
#include "core/engine/subsystem/subsystem_interface.h"

class IInputSubsystem : public IEngineSubSystem
{
public:
    ~IInputSubsystem() override = default;

    // Snapshot the keyboard from the OS and latch the mouse events received
    // since the last poll. Call exactly once per frame, before any query.
    virtual void poll() = 0;

    // Call instead of poll() on frames where the game window does not have
    // focus: every key reads as released (keys held at the moment focus was
    // lost report a release), so typing in another window does not reach
    // the game.
    virtual void releaseAll() = 0;

    // Rising edge: up last frame, down this frame.
    [[nodiscard]] virtual bool pressedThisFrame(EKeyCode key) const = 0;

    // Falling edge: down last frame, up this frame (a completed press).
    [[nodiscard]] virtual bool pressedAndReleased(EKeyCode key) const = 0;

    // Held right now.
    [[nodiscard]] virtual bool isDown(EKeyCode key) const = 0;

    // -- Mouse: fed by the code that owns the window --------------------------

    // The pointer moved to this position, in game viewport pixels.
    virtual void onMouseMoved(const SVector2& viewportPosition) = 0;

    // The pointer left the viewport. The next move starts a new delta
    // instead of jumping from where it left.
    virtual void onMouseLeft() = 0;

    // A button (id 0 .. SInput::MOUSE_BUTTON_COUNT - 1) went down or up.
    virtual void onMouseButton(int button, bool pressed) = 0;

    // Vertical wheel movement, positive = up (notches; fractions on touchpads).
    virtual void onMouseScrolled(float delta) = 0;

    // Raw mouse motion (not clamped by the window edge or the OS cursor).
    // Only used while the mouse is locked.
    virtual void onMouseRawMoved(const SVector2& delta) = 0;

    // Cursor lock (SCursor Locked), set by the viewport host each frame:
    // while locked, the position reads `viewportCenter`, cursor moves are
    // ignored (the host keeps warping it back), and the delta is raw motion.
    virtual void setMouseLocked(bool locked, const SVector2& viewportCenter) = 0;
    [[nodiscard]] virtual bool isMouseLocked() const = 0;

    // -- Mouse: queries, as of the last poll() -------------------------------

    [[nodiscard]] virtual SVector2 getMousePos() const = 0;
    [[nodiscard]] virtual SVector2 getMouseDelta() const = 0;
    [[nodiscard]] virtual bool     isMouseButtonDown(int button) const = 0;
    // Rising / falling edge between the last two polls, as for keys.
    [[nodiscard]] virtual bool     mouseButtonPressedThisFrame(int button) const = 0;
    [[nodiscard]] virtual bool     mouseButtonPressedAndReleased(int button) const = 0;
    [[nodiscard]] virtual float    getMouseScrollDelta() const = 0;   // -1 .. 1
};

#endif // INPUT_SUBSYSTEM_INTERFACE_H
