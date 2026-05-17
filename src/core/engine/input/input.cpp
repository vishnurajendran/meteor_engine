//
// sinput.cpp
//
#include "input.h"
#include "SFML/Window/Keyboard.hpp"

// -- Static storage ----------------------------------------------------------

std::array<bool, SInput::KEY_COUNT> SInput::currentState  = {};
std::array<bool, SInput::KEY_COUNT> SInput::previousState = {};
bool SInput::initialized = false;

// -- poll --------------------------------------------------------------------

void SInput::poll()
{
    previousState = currentState;

    for (int i = 0; i < KEY_COUNT; ++i)
    {
        auto sfKey = static_cast<sf::Keyboard::Key>(i);
        currentState[i] = sf::Keyboard::isKeyPressed(sfKey);
    }

    initialized = true;
}

// -- Edge detection ----------------------------------------------------------

bool SInput::pressedThisFrame(EKeyCode key)
{
    const int i = static_cast<int>(key);
    if (i < 0 || i >= KEY_COUNT) return false;
    return currentState[i] && !previousState[i];
}

bool SInput::pressedAndReleased(EKeyCode key)
{
    const int i = static_cast<int>(key);
    if (i < 0 || i >= KEY_COUNT) return false;
    return !currentState[i] && previousState[i];
}

bool SInput::isDown(EKeyCode key)
{
    const int i = static_cast<int>(key);
    if (i < 0 || i >= KEY_COUNT) return false;
    return currentState[i];
}

// -- Modifier helpers --------------------------------------------------------

bool SInput::isCtrlDown()
{
    return isDown(EKeyCode::LControl) || isDown(EKeyCode::RControl);
}

bool SInput::isShiftDown()
{
    return isDown(EKeyCode::LShift) || isDown(EKeyCode::RShift);
}

bool SInput::isAltDown()
{
    return isDown(EKeyCode::LAlt) || isDown(EKeyCode::RAlt);
}

// -- Key name table ----------------------------------------------------------

const char* SInput::getKeyName(EKeyCode key)
{
    switch (key)
    {
        case EKeyCode::A: return "A";  case EKeyCode::B: return "B";
        case EKeyCode::C: return "C";  case EKeyCode::D: return "D";
        case EKeyCode::E: return "E";  case EKeyCode::F: return "F";
        case EKeyCode::G: return "G";  case EKeyCode::H: return "H";
        case EKeyCode::I: return "I";  case EKeyCode::J: return "J";
        case EKeyCode::K: return "K";  case EKeyCode::L: return "L";
        case EKeyCode::M: return "M";  case EKeyCode::N: return "N";
        case EKeyCode::O: return "O";  case EKeyCode::P: return "P";
        case EKeyCode::Q: return "Q";  case EKeyCode::R: return "R";
        case EKeyCode::S: return "S";  case EKeyCode::T: return "T";
        case EKeyCode::U: return "U";  case EKeyCode::V: return "V";
        case EKeyCode::W: return "W";  case EKeyCode::X: return "X";
        case EKeyCode::Y: return "Y";  case EKeyCode::Z: return "Z";

        case EKeyCode::Num0: return "0"; case EKeyCode::Num1: return "1";
        case EKeyCode::Num2: return "2"; case EKeyCode::Num3: return "3";
        case EKeyCode::Num4: return "4"; case EKeyCode::Num5: return "5";
        case EKeyCode::Num6: return "6"; case EKeyCode::Num7: return "7";
        case EKeyCode::Num8: return "8"; case EKeyCode::Num9: return "9";

        case EKeyCode::Escape:   return "Escape";
        case EKeyCode::LControl: return "LCtrl";
        case EKeyCode::LShift:   return "LShift";
        case EKeyCode::LAlt:     return "LAlt";
        case EKeyCode::LSystem:  return "LSystem";
        case EKeyCode::RControl: return "RCtrl";
        case EKeyCode::RShift:   return "RShift";
        case EKeyCode::RAlt:     return "RAlt";
        case EKeyCode::RSystem:  return "RSystem";
        case EKeyCode::Menu:     return "Menu";

        case EKeyCode::LBracket:   return "[";
        case EKeyCode::RBracket:   return "]";
        case EKeyCode::Semicolon:  return ";";
        case EKeyCode::Comma:      return ",";
        case EKeyCode::Period:     return ".";
        case EKeyCode::Apostrophe: return "'";
        case EKeyCode::Slash:      return "/";
        case EKeyCode::Backslash:  return "\\";
        case EKeyCode::Grave:      return "`";
        case EKeyCode::Equal:      return "=";
        case EKeyCode::Hyphen:     return "-";

        case EKeyCode::Space:     return "Space";
        case EKeyCode::Enter:     return "Enter";
        case EKeyCode::Backspace: return "Backspace";
        case EKeyCode::Tab:       return "Tab";
        case EKeyCode::PageUp:    return "PageUp";
        case EKeyCode::PageDown:  return "PageDown";
        case EKeyCode::End:       return "End";
        case EKeyCode::Home:      return "Home";
        case EKeyCode::Insert:    return "Insert";
        case EKeyCode::Delete:    return "Delete";

        case EKeyCode::Add:      return "+";
        case EKeyCode::Subtract: return "-";
        case EKeyCode::Multiply: return "*";
        case EKeyCode::Divide:   return "/";

        case EKeyCode::Left:  return "Left";
        case EKeyCode::Right: return "Right";
        case EKeyCode::Up:    return "Up";
        case EKeyCode::Down:  return "Down";

        case EKeyCode::Numpad0: return "Num0"; case EKeyCode::Numpad1: return "Num1";
        case EKeyCode::Numpad2: return "Num2"; case EKeyCode::Numpad3: return "Num3";
        case EKeyCode::Numpad4: return "Num4"; case EKeyCode::Numpad5: return "Num5";
        case EKeyCode::Numpad6: return "Num6"; case EKeyCode::Numpad7: return "Num7";
        case EKeyCode::Numpad8: return "Num8"; case EKeyCode::Numpad9: return "Num9";

        case EKeyCode::F1:  return "F1";  case EKeyCode::F2:  return "F2";
        case EKeyCode::F3:  return "F3";  case EKeyCode::F4:  return "F4";
        case EKeyCode::F5:  return "F5";  case EKeyCode::F6:  return "F6";
        case EKeyCode::F7:  return "F7";  case EKeyCode::F8:  return "F8";
        case EKeyCode::F9:  return "F9";  case EKeyCode::F10: return "F10";
        case EKeyCode::F11: return "F11"; case EKeyCode::F12: return "F12";
        case EKeyCode::F13: return "F13"; case EKeyCode::F14: return "F14";
        case EKeyCode::F15: return "F15";

        case EKeyCode::Pause: return "Pause";

        default: return "?";
    }
}