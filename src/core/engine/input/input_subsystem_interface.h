//
// input_subsystem_interface.h
//
// Per-frame keyboard state, as an engine subsystem. The application updates
// it once per frame (poll(), or releaseAll() when its window has no focus)
// before anything reads input. Gameplay code and scripts read it through the
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

    // Snapshot the keyboard from the OS. Call exactly once per frame, before
    // any query.
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
};

#endif // INPUT_SUBSYSTEM_INTERFACE_H
