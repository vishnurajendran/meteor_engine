//
// sfml_input.h
//
// IInputSubsystem backed by SFML's keyboard state, with double-buffered
// per-frame edge detection. EKeyCode mirrors sf::Keyboard::Key value for
// value, so conversion is a static_cast.
//

#ifndef SFML_INPUT_H
#define SFML_INPUT_H

#include <array>

#include "core/engine/input/input_subsystem_interface.h"
#include "core/object/object.h"

class MSfmlInputSubsystem : public MObject, public IInputSubsystem
{
    DEFINE_OBJECT_SUBCLASS(MSfmlInputSubsystem)

public:
    void init() override {}
    void cleanup() override;

    void poll() override;
    void releaseAll() override;

    [[nodiscard]] bool pressedThisFrame(EKeyCode key) const override;
    [[nodiscard]] bool pressedAndReleased(EKeyCode key) const override;
    [[nodiscard]] bool isDown(EKeyCode key) const override;

private:
    static constexpr int KEY_COUNT = static_cast<int>(EKeyCode::KeyCount);

    static bool isValid(EKeyCode key)
    {
        const int i = static_cast<int>(key);
        return i >= 0 && i < KEY_COUNT;
    }

    std::array<bool, KEY_COUNT> currentState  = {};
    std::array<bool, KEY_COUNT> previousState = {};
};

#endif // SFML_INPUT_H
