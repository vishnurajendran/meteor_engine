//
// rml_input.h
//
// Engine key codes <-> RmlUi key identifiers, and the modifier state RmlUi
// wants with every input call. Written against RmlUi's SFML backend tables,
// but from the engine's EKeyCode (which mirrors sf::Keyboard::Key).
//

#ifndef RML_INPUT_H
#define RML_INPUT_H

#include <RmlUi/Core/Input.h>

#include "core/engine/input/input.h"

namespace MRmlInput
{
    Rml::Input::KeyIdentifier toRml(EKeyCode key);
    EKeyCode                  fromRml(int rmlKey);

    // Rml::Input::KeyModifier flags for the keys held right now.
    int currentModifiers();
}

#endif // RML_INPUT_H
