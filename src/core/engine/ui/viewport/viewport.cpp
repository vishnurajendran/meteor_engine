//
// viewport.cpp
//

#include "viewport.h"

#include <algorithm>

#include <RmlUi/Core.h>

#include "SFML/Window/Event.hpp"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/ui/rml/rml_input.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"
#include "core/engine/ui/widget/ui_widget_instance.h"
#include "core/utils/logger.h"

namespace
{
    // Code point -> UTF-8. Control characters are dropped: keys like Enter and
    // Backspace reach RmlUi as key events instead.
    std::string toUtf8(char32_t cp)
    {
        std::string out;
        if (cp < 0x20 || cp == 0x7F || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF)
            return out;
        if (cp < 0x80)
            out += static_cast<char>(cp);
        else if (cp < 0x800)
        {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        else
        {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
        return out;
    }

    int toRmlButton(sf::Mouse::Button button)
    {
        switch (button)
        {
            case sf::Mouse::Button::Left:   return 0;
            case sf::Mouse::Button::Right:  return 1;
            case sf::Mouse::Button::Middle: return 2;
            default:                        return -1;
        }
    }
}

MViewport::MViewport(uint32_t id, IViewportHost* host)
    : id(id), host(host), contextName(SString::format("meteor_viewport_{0}", SString::fromUInt(id)))
{
}

MViewport::~MViewport()
{
    destroyContext();
}

void MViewport::setHost(IViewportHost* newHost)
{
    host = newHost;
    if (host)
        applyCursorToHost();
}

bool MViewport::ensureContext(Rml::RenderInterface* renderInterface)
{
    if (context)
        return true;

    context = Rml::CreateContext(contextName.str(), Rml::Vector2i(width, height), renderInterface);
    if (!context)
    {
        MERROR("MViewport: could not create the RmlUi context " + contextName);
        return false;
    }
    return true;
}

void MViewport::destroyContext()
{
    if (!context)
        return;
    Rml::RemoveContext(contextName.str());
    context = nullptr;
}

void MViewport::setSize(int newWidth, int newHeight)
{
    newWidth  = std::max(1, newWidth);
    newHeight = std::max(1, newHeight);
    if (newWidth == width && newHeight == height)
        return;

    width  = newWidth;
    height = newHeight;
    if (context)
    {
        context->SetDimensions(Rml::Vector2i(width, height));
        markNeedsUpdate();
    }
}

// -- Input ------------------------------------------------------------------------------

int MViewport::currentModifiers() const
{
    return MRmlInput::currentModifiers();
}

bool MViewport::processMouseMove(int x, int y)
{
    if (!context)
        return false;
    context->ProcessMouseMove(x, y, currentModifiers());
    markNeedsUpdate();
    return isConsumingMouse();
}

bool MViewport::processMouseButton(int button, bool pressed)
{
    if (!context || button < 0 || button > 2)
        return false;

    // A release is delivered only for a press the UI saw, so a drag that
    // started in the game doesn't end as a UI click.
    if (!pressed && !mouseButtonsDown[button])
        return false;

    const bool consumedBefore = isConsumingMouse();
    if (pressed)
        context->ProcessMouseButtonDown(button, currentModifiers());
    else
        context->ProcessMouseButtonUp(button, currentModifiers());
    mouseButtonsDown[button] = pressed;
    markNeedsUpdate();
    return consumedBefore || isConsumingMouse();
}

bool MViewport::processMouseWheel(float deltaX, float deltaY)
{
    if (!context)
        return false;
    // RmlUi: positive = scroll down / right. SFML: positive = up / left.
    const bool notConsumed = context->ProcessMouseWheel(Rml::Vector2f(-deltaX, -deltaY), currentModifiers());
    markNeedsUpdate();
    return !notConsumed || isConsumingMouse();
}

void MViewport::processMouseLeave()
{
    if (!context)
        return;
    context->ProcessMouseLeave();
    markNeedsUpdate();
}

bool MViewport::processKey(EKeyCode key, bool pressed)
{
    if (!context)
        return false;
    const Rml::Input::KeyIdentifier rmlKey = MRmlInput::toRml(key);
    if (rmlKey == Rml::Input::KI_UNKNOWN)
        return false;

    const bool notConsumed = pressed ? context->ProcessKeyDown(rmlKey, currentModifiers())
                                     : context->ProcessKeyUp(rmlKey, currentModifiers());
    markNeedsUpdate();
    return !notConsumed || isConsumingKeyboard();
}

bool MViewport::processText(const SString& utf8)
{
    if (!context || utf8.empty())
        return false;
    // Only typed into a text field; otherwise it is the game's key press.
    if (!isConsumingKeyboard())
        return false;
    context->ProcessTextInput(utf8.str());
    markNeedsUpdate();
    return true;
}

void MViewport::releaseAll()
{
    if (!context)
        return;
    for (int button = 0; button < 3; ++button)
        if (mouseButtonsDown[button])
            processMouseButton(button, false);
}

bool MViewport::processWindowEvent(const sf::Event& event, SVector2 origin, SVector2 scale)
{
    // A locked cursor is held at the centre for mouse-look; those positions
    // and clicks are the game's, not the UI's. Keys and text still go through.
    if (cursorState == ECursorState::Locked
        && (event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseButtonPressed>()
            || event.is<sf::Event::MouseButtonReleased>() || event.is<sf::Event::MouseWheelScrolled>()))
        return false;

    auto toViewport = [&](sf::Vector2i position, int& x, int& y)
    {
        x = static_cast<int>((static_cast<float>(position.x) - origin.x) * scale.x);
        y = static_cast<int>((static_cast<float>(position.y) - origin.y) * scale.y);
    };

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
    {
        int x = 0, y = 0;
        toViewport(moved->position, x, y);
        return processMouseMove(x, y);
    }
    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        int x = 0, y = 0;
        toViewport(pressed->position, x, y);
        processMouseMove(x, y);
        return processMouseButton(toRmlButton(pressed->button), true);
    }
    if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
    {
        int x = 0, y = 0;
        toViewport(released->position, x, y);
        processMouseMove(x, y);
        return processMouseButton(toRmlButton(released->button), false);
    }
    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        const bool vertical = wheel->wheel == sf::Mouse::Wheel::Vertical;
        return processMouseWheel(vertical ? 0.0f : wheel->delta, vertical ? wheel->delta : 0.0f);
    }
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
        return processKey(static_cast<EKeyCode>(static_cast<int>(key->code)), true);
    if (const auto* key = event.getIf<sf::Event::KeyReleased>())
        return processKey(static_cast<EKeyCode>(static_cast<int>(key->code)), false);
    if (const auto* text = event.getIf<sf::Event::TextEntered>())
    {
        const std::string utf8 = toUtf8(text->unicode);
        return !utf8.empty() && processText(SString(utf8));
    }
    if (event.is<sf::Event::MouseLeft>())
    {
        processMouseLeave();
        return false;
    }
    if (event.is<sf::Event::FocusLost>())
    {
        releaseAll();
        return false;
    }
    return false;
}

bool MViewport::isConsumingMouse() const
{
    if (!context)
        return false;

    // Over an element - not the context's root, and not a widget's document
    // (body): a full-screen HUD's transparent background doesn't block the
    // game. pointer-events: none elements are never hovered.
    const Rml::Element* hover = context->GetHoverElement();
    if (hover)
    {
        const Rml::ElementDocument* owner = hover->GetOwnerDocument();
        if (owner && static_cast<const Rml::Element*>(owner) != hover)
            return true;
    }
    for (bool down : mouseButtonsDown)
        if (down)
            return true;
    return false;
}

bool MViewport::isConsumingKeyboard() const
{
    if (!context)
        return false;

    // A text field has focus. Buttons, checkboxes and the like don't take the
    // keyboard from the game.
    const Rml::Element* focus = context->GetFocusElement();
    if (!focus)
        return false;
    const Rml::String& tag = focus->GetTagName();
    if (tag == "textarea")
        return true;
    if (tag == "input")
    {
        const Rml::String type = focus->GetAttribute<Rml::String>("type", "text");
        return type == "text" || type == "password" || type == "search" || type == "email" || type == "number";
    }
    return false;
}

// -- Cursor -----------------------------------------------------------------------------

void MViewport::setCursorVisible(bool visible)
{
    cursorVisible = visible;
    applyCursorToHost();
}

void MViewport::setCursorState(ECursorState state)
{
    // Locked: the pointer belongs to the game (mouse-look). The UI lets go of
    // anything it was hovering or holding, and gets no pointer events until
    // the cursor is unlocked.
    if (state == ECursorState::Locked && cursorState != ECursorState::Locked)
    {
        releaseAll();
        processMouseLeave();
    }
    cursorState = state;
    applyCursorToHost();
}

void MViewport::resetCursor()
{
    cursorVisible = true;
    cursorState   = ECursorState::Free;
    applyCursorToHost();
}

void MViewport::applyCursorToHost() const
{
    if (host)
        host->applyCursor(cursorVisible, cursorState);
}

// -- Widgets ----------------------------------------------------------------------------

void MViewport::addWidgetEntry(uint32_t widgetId, int zOrder, uint64_t addSequence)
{
    removeWidgetEntry(widgetId);
    const SWidgetEntry entry{ widgetId, zOrder, addSequence };
    const auto at = std::upper_bound(widgets.begin(), widgets.end(), entry,
        [](const SWidgetEntry& a, const SWidgetEntry& b)
        {
            return a.zOrder != b.zOrder ? a.zOrder < b.zOrder : a.addSequence < b.addSequence;
        });
    widgets.insert(at, entry);
}

void MViewport::removeWidgetEntry(uint32_t widgetId)
{
    std::erase_if(widgets, [widgetId](const SWidgetEntry& entry) { return entry.widgetId == widgetId; });
}

// -- Script API -------------------------------------------------------------------------

static MViewportSubsystem* uiSubsystem()
{
    return MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>();
}

void MViewport::addWidget(const MUIWidget& widget, int index)
{
    if (!widget.isValid())
    {
        MWARN("MViewport.addWidget: the widget is not valid (destroyed, or creation failed)");
        return;
    }
    if (widget.getViewportId() != id)
    {
        MWARN(SString::format("MViewport.addWidget: '{0}' belongs to another viewport", widget.getName()));
        return;
    }
    if (auto* ui = uiSubsystem())
        ui->addToViewport(widget, index);
}

void MViewport::removeWidget(const MUIWidget& widget)
{
    if (widget.getViewportId() != id)
        return;
    if (auto* ui = uiSubsystem())
        ui->removeFromViewport(widget);
}

MUIWidget MViewport::getCurrentWidget(int index) const
{
    const auto* ui = uiSubsystem();
    if (!ui)
        return {};
    // Sorted by (layer, add order): the last match is the top of the layer.
    for (auto it = widgets.rbegin(); it != widgets.rend(); ++it)
        if (it->zOrder == index)
            if (const SUIWidgetInstance* widget = ui->getInstance(it->widgetId))
                return widget->handle();
    return {};
}

void MViewport::clear()
{
    auto* ui = uiSubsystem();
    if (!ui)
        return;
    // Copy: removing edits the list.
    const std::vector<SWidgetEntry> entries = widgets;
    for (const SWidgetEntry& entry : entries)
        if (const SUIWidgetInstance* widget = ui->getInstance(entry.widgetId))
            ui->removeFromViewport(widget->handle());
}
