//
// sfml_input.cpp
//

#include "core/engine/input/impl_sfml/sfml_input.h"

#include <algorithm>

#include "SFML/Window/Keyboard.hpp"

// The scroll query's range: a frame with several notches reads as one.
static constexpr float MAX_SCROLL_PER_FRAME = 1.0f;

void MSfmlInputSubsystem::cleanup()
{
    currentState.fill(false);
    previousState.fill(false);
    resetMouse();
    previousButtons.fill(false);
    mouseLocked = false;
    pendingPosition = framePosition = SVector2(0.0f);
}

void MSfmlInputSubsystem::poll()
{
    previousState = currentState;
    for (int i = 0; i < KEY_COUNT; ++i)
        currentState[i] = sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(i));

    previousButtons = frameButtons;
    for (int i = 0; i < SInput::MOUSE_BUTTON_COUNT; ++i)
    {
        frameButtons[i]        = mouseHeld[i] || mousePressedInFrame[i];
        mousePressedInFrame[i] = false;
    }
    framePosition = pendingPosition;
    frameDelta    = pendingDelta;
    frameScroll   = std::clamp(pendingScroll, -MAX_SCROLL_PER_FRAME, MAX_SCROLL_PER_FRAME);
    pendingDelta  = SVector2(0.0f);
    pendingScroll = 0.0f;
}

void MSfmlInputSubsystem::releaseAll()
{
    previousState = currentState;
    currentState.fill(false);
    // The position stays where it was; buttons, movement and scroll stop.
    // Buttons held when focus went report a release, like keys.
    previousButtons = frameButtons;
    resetMouse();
}

void MSfmlInputSubsystem::resetMouse()
{
    mouseHeld.fill(false);
    mousePressedInFrame.fill(false);
    frameButtons.fill(false);
    pendingDelta  = frameDelta  = SVector2(0.0f);
    pendingScroll = frameScroll = 0.0f;
    hasPointer    = false;
}

// -- Mouse events ------------------------------------------------------------

void MSfmlInputSubsystem::onMouseMoved(const SVector2& viewportPosition)
{
    // Locked: the host warps the cursor back every frame; those moves aren't the player's.
    if (mouseLocked)
        return;
    if (hasPointer)
        pendingDelta += viewportPosition - pendingPosition;
    pendingPosition = viewportPosition;
    hasPointer      = true;
}

void MSfmlInputSubsystem::onMouseLeft()
{
    hasPointer = false;
}

void MSfmlInputSubsystem::onMouseButton(int button, bool pressed)
{
    if (!isValidMouseButton(button))
        return;
    mouseHeld[button] = pressed;
    if (pressed)
        mousePressedInFrame[button] = true;
}

void MSfmlInputSubsystem::onMouseScrolled(float delta)
{
    pendingScroll += delta;
}

void MSfmlInputSubsystem::onMouseRawMoved(const SVector2& delta)
{
    if (mouseLocked)
        pendingDelta += delta;
}

void MSfmlInputSubsystem::setMouseLocked(bool locked, const SVector2& viewportCenter)
{
    if (locked)
        pendingPosition = viewportCenter;
    if (locked == mouseLocked)
        return;
    mouseLocked = locked;
    // Either way the next cursor move starts fresh, rather than counting the
    // jump to or from the centre.
    hasPointer = false;
    if (!locked)
        pendingDelta = SVector2(0.0f);
}

bool MSfmlInputSubsystem::isMouseButtonDown(int button) const
{
    return isValidMouseButton(button) && frameButtons[button];
}

bool MSfmlInputSubsystem::mouseButtonPressedThisFrame(int button) const
{
    return isValidMouseButton(button) && frameButtons[button] && !previousButtons[button];
}

bool MSfmlInputSubsystem::mouseButtonPressedAndReleased(int button) const
{
    return isValidMouseButton(button) && !frameButtons[button] && previousButtons[button];
}

int MSfmlInputSubsystem::toMouseButtonId(sf::Mouse::Button button)
{
    switch (button)
    {
        case sf::Mouse::Button::Left:   return static_cast<int>(EMouseButton::Left);
        case sf::Mouse::Button::Middle: return static_cast<int>(EMouseButton::Middle);
        case sf::Mouse::Button::Right:  return static_cast<int>(EMouseButton::Right);
        case sf::Mouse::Button::Extra1: return 3;
        case sf::Mouse::Button::Extra2: return 4;
        default:                        return -1;
    }
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
