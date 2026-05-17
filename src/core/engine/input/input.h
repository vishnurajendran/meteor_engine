//
// sinput.h
//
// Core input abstraction. Wraps SFML keyboard state with double-buffered
// per-frame edge detection. Call poll() once at the start of each frame
// before any input queries.
//
#pragma once
#ifndef SINPUT_H
#define SINPUT_H

#include <array>
#include <string>

// Key codes mirroring sf::Keyboard::Key (SFML 3.0.x) value-for-value.
// Conversion to/from sf::Keyboard::Key is a zero-cost static_cast.
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

class SInput
{
public:
    // Snapshot the current keyboard state from the OS. Call exactly once per
    // frame, before any pressedThisFrame / pressedAndReleased / isDown queries.
    static void poll();

    // Rising edge -- the key transitioned from up to down this frame.
    static bool pressedThisFrame(EKeyCode key);

    // Falling edge -- the key transitioned from down to up this frame
    // (i.e. the user completed a press-and-release gesture).
    static bool pressedAndReleased(EKeyCode key);

    // Raw held state -- the key is physically down right now.
    static bool isDown(EKeyCode key);

    // Modifier convenience helpers (left OR right counts).
    static bool isCtrlDown();
    static bool isShiftDown();
    static bool isAltDown();

    // Human-readable name for a key code (e.g. EKeyCode::S -> "S").
    static const char* getKeyName(EKeyCode key);

private:
    static constexpr int KEY_COUNT = static_cast<int>(EKeyCode::KeyCount);

    static std::array<bool, KEY_COUNT> currentState;
    static std::array<bool, KEY_COUNT> previousState;
    static bool initialized;
};

#endif // SINPUT_H