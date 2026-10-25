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

SCRIPT_BIND_STRUCT()
struct SInput
{
public:
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

    // Human-readable name for a key code (e.g. EKeyCode::S -> "S").
    SCRIPT_BIND_FUNC_STATIC()
    static const char* getKeyName(EKeyCode key);
};

#endif // SINPUT_H