//
// sfml_input.cpp
//

#include "core/engine/input/impl_sfml/sfml_input.h"

#include "SFML/Window/Keyboard.hpp"

void MSfmlInputSubsystem::cleanup()
{
    currentState.fill(false);
    previousState.fill(false);
}

void MSfmlInputSubsystem::poll()
{
    previousState = currentState;
    for (int i = 0; i < KEY_COUNT; ++i)
        currentState[i] = sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(i));
}

void MSfmlInputSubsystem::releaseAll()
{
    previousState = currentState;
    currentState.fill(false);
}

bool MSfmlInputSubsystem::pressedThisFrame(EKeyCode key) const
{
    if (!isValid(key)) return false;
    const int i = static_cast<int>(key);
    return currentState[i] && !previousState[i];
}

bool MSfmlInputSubsystem::pressedAndReleased(EKeyCode key) const
{
    if (!isValid(key)) return false;
    const int i = static_cast<int>(key);
    return !currentState[i] && previousState[i];
}

bool MSfmlInputSubsystem::isDown(EKeyCode key) const
{
    if (!isValid(key)) return false;
    return currentState[static_cast<int>(key)];
}
