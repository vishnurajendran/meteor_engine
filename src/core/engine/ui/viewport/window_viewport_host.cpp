//
// window_viewport_host.cpp
//

#include "window_viewport_host.h"

#include "core/engine/input/input_subsystem_interface.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/window/simple/windowing.h"

void MWindowViewportHost::applyCursor(bool visible, ECursorState state)
{
    cursorState = state;
    if (!window)
        return;
    window->setCursorVisible(visible && state != ECursorState::Locked);
    window->setCursorGrabbed(state != ECursorState::Free);
}

void MWindowViewportHost::update()
{
    auto* input = MEngineSubsystemRegistry::getSubsystem<IInputSubsystem>();
    if (!window || !input)
        return;

    const sf::Vector2u size = window->getSize();
    const sf::Vector2i center(static_cast<int>(size.x / 2), static_cast<int>(size.y / 2));
    const bool locked = cursorState == ECursorState::Locked && window->hasFocus();
    if (locked)
        window->setCursorPosition(center);
    input->setMouseLocked(locked, SVector2(static_cast<float>(center.x), static_cast<float>(center.y)));
}

void MWindowViewportHost::setCursorShape(const SString& cursorName)
{
    if (!window)
        return;

    // RmlUi's cursor names are CSS ones.
    sf::Cursor::Type type = sf::Cursor::Type::Arrow;
    if (cursorName == "pointer")
        type = sf::Cursor::Type::Hand;
    else if (cursorName == "text")
        type = sf::Cursor::Type::Text;
    else if (cursorName == "move")
        type = sf::Cursor::Type::SizeAll;
    else if (cursorName == "not-allowed")
        type = sf::Cursor::Type::NotAllowed;
    else if (cursorName == "ew-resize" || cursorName == "col-resize")
        type = sf::Cursor::Type::SizeHorizontal;
    else if (cursorName == "ns-resize" || cursorName == "row-resize")
        type = sf::Cursor::Type::SizeVertical;
    else if (cursorName == "wait" || cursorName == "progress")
        type = sf::Cursor::Type::Wait;
    window->setSystemCursor(type);
}
