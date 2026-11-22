//
// sinput.h
//
// Input facade for gameplay code and scripts. Every query reads the input
// subsystem (IInputSubsystem, input_subsystem_interface.h), which the
// application updates once per frame; with no input subsystem registered,
// every key reads as up.
//
#pragma once
#ifndef SINPUT_H
#define SINPUT_H

#include <array>
#include <string>

#include "core/engine/scripting/script_binding_macros.h"
#include "core/utils/glmhelper.h"

// Key codes mirroring sf::Keyboard::Key (SFML 3.0.x) value-for-value.
// Conversion to/from sf::Keyboard::Key is a zero-cost static_cast.
SCRIPT_BIND_ENUM()
enum class EKeyCode : int
{
    Unknown = -1,

    // Letters
    A = 0, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Top-row digits
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Control
    Escape,
    LControl, LShift, LAlt, LSystem,
    RControl, RShift, RAlt, RSystem,
    Menu,

    // Punctuation / symbols
    LBracket, RBracket,
    Semicolon, Comma, Period, Apostrophe, Slash, Backslash,
    Grave, Equal, Hyphen,

    // Whitespace / editing
    Space, Enter, Backspace, Tab,
    PageUp, PageDown, End, Home,
    Insert, Delete,

    // Arithmetic (numpad operators)
    Add, Subtract, Multiply, Divide,

    // Arrows
    Left, Right, Up, Down,

    // Numpad digits
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,

    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10,
    F11, F12, F13, F14, F15,

    Pause,

    KeyCount   // sentinel -- must remain last
};

// Names for the common mouse buttons. The value is the button's id; buttons
// past these (side buttons: 3, 4) are read by number:
//   SInput.getMouseButton(EMouseButton.Left)    SInput.getMouseButton(3)
SCRIPT_BIND_ENUM()
enum class EMouseButton : int
{
    Left   = 0,
    Middle = 1,
    Right  = 2,
};

SCRIPT_BIND_STRUCT()
struct SInput
{
public:
    // Mouse button ids 0 .. MOUSE_BUTTON_COUNT - 1: the three EMouseButton
    // names, then the two side buttons (3, 4).
    static constexpr int MOUSE_BUTTON_COUNT = 5;

    // Rising edge -- the key transitioned from up to down this frame.
    SCRIPT_BIND_FUNC_STATIC()
    static bool pressedThisFrame(EKeyCode key);

    // Falling edge -- the key transitioned from down to up this frame
    // (i.e. the user completed a press-and-release gesture).
    SCRIPT_BIND_FUNC_STATIC()
    static bool pressedAndReleased(EKeyCode key);

    // Raw held state -- the key is physically down right now.
    SCRIPT_BIND_FUNC_STATIC()
    static bool isDown(EKeyCode key);

    // Modifier convenience helpers (left OR right counts).
    SCRIPT_BIND_FUNC_STATIC()
    static bool isCtrlDown();

    SCRIPT_BIND_FUNC_STATIC()
    static bool isShiftDown();

    SCRIPT_BIND_FUNC_STATIC()
    static bool isAltDown();

    // True while the game UI has the mouse: over a UI element (not a widget's
    // transparent background) or dragging one. Key and mouse queries keep
    // working either way - check this to ignore input the UI took.
    SCRIPT_BIND_FUNC_STATIC()
    static bool isUIConsumingMouse();

    // True while a text field in the game UI has keyboard focus.
    SCRIPT_BIND_FUNC_STATIC()
    static bool isUIConsumingKeyboard();

    // -- Mouse --------------------------------------------------------------
    // Over the game viewport, in its pixels from the top-left corner (in the
    // editor, the Scene view's game image). Read once per frame, like keys.

    // Where the pointer is. Keeps its last value while the pointer is
    // outside the viewport.
    SCRIPT_BIND_FUNC_STATIC()
    static SVector2 getMousePos();

    // How far the pointer moved since last frame, in pixels (+x right,
    // +y down). Zero for the frame the pointer re-enters the viewport.
    SCRIPT_BIND_FUNC_STATIC()
    static SVector2 getMouseDelta();

    // Buttons are EMouseButton, like keys are EKeyCode. Buttons past the
    // named ones are read by id: Lua passes the number (SInput.getMouseButton(3));
    // C++ casts (static_cast<EMouseButton>(3)). Unknown ids read as up.

    // The button is held. A click shorter than a frame still reads as held
    // for one frame.
    SCRIPT_BIND_FUNC_STATIC()
    static bool getMouseButton(EMouseButton button);

    // Rising edge -- the button went from up to down this frame.
    SCRIPT_BIND_FUNC_STATIC()
    static bool mouseButtonPressedThisFrame(EMouseButton button);

    // Falling edge -- the button went from down to up this frame (a
    // completed click, ending this frame).
    SCRIPT_BIND_FUNC_STATIC()
    static bool mouseButtonPressedAndReleased(EMouseButton button);

    // Vertical wheel movement this frame, -1 .. 1: positive = scrolled up
    // (away from you), 0 = no scroll. Several notches in one frame clamp
    // to +/-1; touchpads give fractions.
    SCRIPT_BIND_FUNC_STATIC()
    static float getMouseScrollDelta();

    // Human-readable name for a key code (e.g. EKeyCode::S -> "S").
    SCRIPT_BIND_FUNC_STATIC()
    static const char* getKeyName(EKeyCode key);
};

#endif // SINPUT_H
