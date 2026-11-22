//
// sfml_input.h
//
// IInputSubsystem backed by SFML's keyboard state, with double-buffered
// per-frame edge detection. EKeyCode mirrors sf::Keyboard::Key value for
// value, so conversion is a static_cast. Mouse state comes from window
// events (the application feeds them; toMouseButtonId maps SFML's buttons).
//

#ifndef SFML_INPUT_H
#define SFML_INPUT_H

#include <array>

#include "SFML/Window/Mouse.hpp"
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

    void onMouseMoved(const SVector2& viewportPosition) override;
    void onMouseLeft() override;
    void onMouseButton(int button, bool pressed) override;
    void onMouseScrolled(float delta) override;
    void onMouseRawMoved(const SVector2& delta) override;
    void setMouseLocked(bool locked, const SVector2& viewportCenter) override;
    [[nodiscard]] bool isMouseLocked() const override { return mouseLocked; }

    [[nodiscard]] SVector2 getMousePos() const override { return framePosition; }
    [[nodiscard]] SVector2 getMouseDelta() const override { return frameDelta; }
    [[nodiscard]] bool     isMouseButtonDown(int button) const override;
    [[nodiscard]] bool     mouseButtonPressedThisFrame(int button) const override;
    [[nodiscard]] bool     mouseButtonPressedAndReleased(int button) const override;
    [[nodiscard]] float    getMouseScrollDelta() const override { return frameScroll; }

    // SFML's button -> our id (Left 0, Middle 1, Right 2, Extra1 3, Extra2 4).
    // SFML orders them Left, Right, Middle, so this is not a cast.
    static int toMouseButtonId(sf::Mouse::Button button);

private:
    static constexpr int KEY_COUNT = static_cast<int>(EKeyCode::KeyCount);

    static bool isValid(EKeyCode key)
    {
        const int i = static_cast<int>(key);
        return i >= 0 && i < KEY_COUNT;
    }

    std::array<bool, KEY_COUNT> currentState  = {};
    std::array<bool, KEY_COUNT> previousState = {};

    static bool isValidMouseButton(int button) { return button >= 0 && button < SInput::MOUSE_BUTTON_COUNT; }
    void resetMouse();

    // Live, from events since the last poll().
    std::array<bool, SInput::MOUSE_BUTTON_COUNT> mouseHeld          = {};
    std::array<bool, SInput::MOUSE_BUTTON_COUNT> mousePressedInFrame = {};   // so a sub-frame click still shows
    SVector2 pendingPosition  = SVector2(0.0f);
    SVector2 pendingDelta     = SVector2(0.0f);
    float    pendingScroll    = 0.0f;
    bool     hasPointer       = false;   // pendingPosition is a real pointer position
    bool     mouseLocked      = false;

    // Latched by poll(); what queries return.
    std::array<bool, SInput::MOUSE_BUTTON_COUNT> frameButtons    = {};
    std::array<bool, SInput::MOUSE_BUTTON_COUNT> previousButtons = {};   // frameButtons one poll earlier
    SVector2 framePosition = SVector2(0.0f);
    SVector2 frameDelta    = SVector2(0.0f);
    float    frameScroll   = 0.0f;
};

#endif // SFML_INPUT_H
