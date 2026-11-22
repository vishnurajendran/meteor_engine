//
// rml_input.cpp
//

#include "rml_input.h"

#include <array>

#include "SFML/Window/Keyboard.hpp"

namespace
{
    using KI = Rml::Input::KeyIdentifier;

    struct SKeyPair
    {
        EKeyCode key;
        KI       rml;
    };

    constexpr SKeyPair KEY_TABLE[] = {
        { EKeyCode::A, KI::KI_A }, { EKeyCode::B, KI::KI_B }, { EKeyCode::C, KI::KI_C },
        { EKeyCode::D, KI::KI_D }, { EKeyCode::E, KI::KI_E }, { EKeyCode::F, KI::KI_F },
        { EKeyCode::G, KI::KI_G }, { EKeyCode::H, KI::KI_H }, { EKeyCode::I, KI::KI_I },
        { EKeyCode::J, KI::KI_J }, { EKeyCode::K, KI::KI_K }, { EKeyCode::L, KI::KI_L },
        { EKeyCode::M, KI::KI_M }, { EKeyCode::N, KI::KI_N }, { EKeyCode::O, KI::KI_O },
        { EKeyCode::P, KI::KI_P }, { EKeyCode::Q, KI::KI_Q }, { EKeyCode::R, KI::KI_R },
        { EKeyCode::S, KI::KI_S }, { EKeyCode::T, KI::KI_T }, { EKeyCode::U, KI::KI_U },
        { EKeyCode::V, KI::KI_V }, { EKeyCode::W, KI::KI_W }, { EKeyCode::X, KI::KI_X },
        { EKeyCode::Y, KI::KI_Y }, { EKeyCode::Z, KI::KI_Z },

        { EKeyCode::Num0, KI::KI_0 }, { EKeyCode::Num1, KI::KI_1 }, { EKeyCode::Num2, KI::KI_2 },
        { EKeyCode::Num3, KI::KI_3 }, { EKeyCode::Num4, KI::KI_4 }, { EKeyCode::Num5, KI::KI_5 },
        { EKeyCode::Num6, KI::KI_6 }, { EKeyCode::Num7, KI::KI_7 }, { EKeyCode::Num8, KI::KI_8 },
        { EKeyCode::Num9, KI::KI_9 },

        { EKeyCode::Escape,   KI::KI_ESCAPE },
        { EKeyCode::LControl, KI::KI_LCONTROL }, { EKeyCode::LShift, KI::KI_LSHIFT },
        { EKeyCode::LAlt,     KI::KI_LMENU },    { EKeyCode::LSystem, KI::KI_LWIN },
        { EKeyCode::RControl, KI::KI_RCONTROL }, { EKeyCode::RShift, KI::KI_RSHIFT },
        { EKeyCode::RAlt,     KI::KI_RMENU },    { EKeyCode::RSystem, KI::KI_RWIN },
        { EKeyCode::Menu,     KI::KI_APPS },

        { EKeyCode::LBracket,   KI::KI_OEM_4 },      { EKeyCode::RBracket,  KI::KI_OEM_6 },
        { EKeyCode::Semicolon,  KI::KI_OEM_1 },      { EKeyCode::Comma,     KI::KI_OEM_COMMA },
        { EKeyCode::Period,     KI::KI_OEM_PERIOD }, { EKeyCode::Apostrophe, KI::KI_OEM_7 },
        { EKeyCode::Slash,      KI::KI_OEM_2 },      { EKeyCode::Backslash, KI::KI_OEM_5 },
        { EKeyCode::Grave,      KI::KI_OEM_3 },      { EKeyCode::Equal,     KI::KI_OEM_PLUS },
        { EKeyCode::Hyphen,     KI::KI_OEM_MINUS },

        { EKeyCode::Space,     KI::KI_SPACE }, { EKeyCode::Enter,  KI::KI_RETURN },
        { EKeyCode::Backspace, KI::KI_BACK },  { EKeyCode::Tab,    KI::KI_TAB },
        { EKeyCode::PageUp,    KI::KI_PRIOR }, { EKeyCode::PageDown, KI::KI_NEXT },
        { EKeyCode::End,       KI::KI_END },   { EKeyCode::Home,   KI::KI_HOME },
        { EKeyCode::Insert,    KI::KI_INSERT }, { EKeyCode::Delete, KI::KI_DELETE },

        { EKeyCode::Add,      KI::KI_ADD },      { EKeyCode::Subtract, KI::KI_SUBTRACT },
        { EKeyCode::Multiply, KI::KI_MULTIPLY }, { EKeyCode::Divide,   KI::KI_DIVIDE },

        { EKeyCode::Left, KI::KI_LEFT }, { EKeyCode::Right, KI::KI_RIGHT },
        { EKeyCode::Up,   KI::KI_UP },   { EKeyCode::Down,  KI::KI_DOWN },

        { EKeyCode::Numpad0, KI::KI_NUMPAD0 }, { EKeyCode::Numpad1, KI::KI_NUMPAD1 },
        { EKeyCode::Numpad2, KI::KI_NUMPAD2 }, { EKeyCode::Numpad3, KI::KI_NUMPAD3 },
        { EKeyCode::Numpad4, KI::KI_NUMPAD4 }, { EKeyCode::Numpad5, KI::KI_NUMPAD5 },
        { EKeyCode::Numpad6, KI::KI_NUMPAD6 }, { EKeyCode::Numpad7, KI::KI_NUMPAD7 },
        { EKeyCode::Numpad8, KI::KI_NUMPAD8 }, { EKeyCode::Numpad9, KI::KI_NUMPAD9 },

        { EKeyCode::F1,  KI::KI_F1 },  { EKeyCode::F2,  KI::KI_F2 },  { EKeyCode::F3,  KI::KI_F3 },
        { EKeyCode::F4,  KI::KI_F4 },  { EKeyCode::F5,  KI::KI_F5 },  { EKeyCode::F6,  KI::KI_F6 },
        { EKeyCode::F7,  KI::KI_F7 },  { EKeyCode::F8,  KI::KI_F8 },  { EKeyCode::F9,  KI::KI_F9 },
        { EKeyCode::F10, KI::KI_F10 }, { EKeyCode::F11, KI::KI_F11 }, { EKeyCode::F12, KI::KI_F12 },
        { EKeyCode::F13, KI::KI_F13 }, { EKeyCode::F14, KI::KI_F14 }, { EKeyCode::F15, KI::KI_F15 },

        { EKeyCode::Pause, KI::KI_PAUSE },
    };

    constexpr size_t KEY_COUNT = static_cast<size_t>(EKeyCode::KeyCount);

    // Indexed by EKeyCode.
    const std::array<KI, KEY_COUNT>& toRmlTable()
    {
        static const auto table = []
        {
            std::array<KI, KEY_COUNT> t{};
            t.fill(KI::KI_UNKNOWN);
            for (const auto& pair : KEY_TABLE)
                t[static_cast<size_t>(pair.key)] = pair.rml;
            return t;
        }();
        return table;
    }

    bool held(sf::Keyboard::Key key) { return sf::Keyboard::isKeyPressed(key); }
}

Rml::Input::KeyIdentifier MRmlInput::toRml(EKeyCode key)
{
    const int index = static_cast<int>(key);
    if (index < 0 || index >= static_cast<int>(KEY_COUNT))
        return KI::KI_UNKNOWN;
    return toRmlTable()[static_cast<size_t>(index)];
}

EKeyCode MRmlInput::fromRml(int rmlKey)
{
    for (const auto& pair : KEY_TABLE)
        if (static_cast<int>(pair.rml) == rmlKey)
            return pair.key;
    return EKeyCode::Unknown;
}

int MRmlInput::currentModifiers()
{
    int modifiers = 0;
    if (held(sf::Keyboard::Key::LControl) || held(sf::Keyboard::Key::RControl))
        modifiers |= Rml::Input::KM_CTRL;
    if (held(sf::Keyboard::Key::LShift) || held(sf::Keyboard::Key::RShift))
        modifiers |= Rml::Input::KM_SHIFT;
    if (held(sf::Keyboard::Key::LAlt) || held(sf::Keyboard::Key::RAlt))
        modifiers |= Rml::Input::KM_ALT;
    if (held(sf::Keyboard::Key::LSystem) || held(sf::Keyboard::Key::RSystem))
        modifiers |= Rml::Input::KM_META;
    return modifiers;
}
