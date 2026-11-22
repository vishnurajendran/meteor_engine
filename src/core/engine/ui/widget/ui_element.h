//
// ui_element.h
//
// MUIElement - a script-facing handle to one element of a widget, found by
// id with MUIWidget:findElement.
//
//   local label = self.widget:findElement("health")
//   label:setText("100")
//   label:setClass("low", hp < 20)
//   label:setProperty("width", "50%")
//
// It is { widget, element id } and resolves within that widget's own
// elements, so it keeps working when the widget's .rml is hot-reloaded (the
// element is found again by id). A handle whose widget or element is gone is
// invalid and its calls do nothing.
//

#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include <cstdint>

#include "core/engine/scripting/script_binding_macros.h"
#include "core/utils/sstring.h"
#include "ui_widget.h"

namespace Rml { class Element; }

SCRIPT_BIND_CLASS()
class MUIElement
{
public:
    MUIElement() = default;
    MUIElement(const MUIWidget& widget, const SString& elementId) : widget(widget), elementId(elementId) {}

    SCRIPT_BIND_FUNC()
    bool isValid() const;

    SCRIPT_BIND_FUNC()
    SString getId() const { return elementId; }

    // The widget this element belongs to.
    SCRIPT_BIND_FUNC()
    MUIWidget getWidget() const { return widget; }

    // Plain text; markup characters are escaped, so "<b>" shows as typed.
    SCRIPT_BIND_FUNC()
    void setText(const SString& text);

    SCRIPT_BIND_FUNC()
    SString getText() const;

    // RML markup as the element's content. Elements created this way can be
    // found with findElement; ones they replace can no longer be.
    SCRIPT_BIND_FUNC()
    void setInnerRml(const SString& rml);

    SCRIPT_BIND_FUNC()
    void setClass(const SString& className, bool enabled);

    SCRIPT_BIND_FUNC()
    bool hasClass(const SString& className) const;

    // An RCSS property on this element, e.g. setProperty("width", "50%").
    // False if RmlUi rejects the name or value.
    SCRIPT_BIND_FUNC()
    bool setProperty(const SString& name, const SString& value);

    SCRIPT_BIND_FUNC()
    void removeProperty(const SString& name);

    SCRIPT_BIND_FUNC()
    void setAttribute(const SString& name, const SString& value);

    // Empty when the attribute isn't set.
    SCRIPT_BIND_FUNC()
    SString getAttribute(const SString& name) const;

    // Form controls (input, textarea, select): the current value.
    SCRIPT_BIND_FUNC()
    SString getValue() const;

    SCRIPT_BIND_FUNC()
    void setValue(const SString& value);

    // display: none while hidden; showing again restores the stylesheet's.
    SCRIPT_BIND_FUNC()
    void setVisible(bool visible);

    SCRIPT_BIND_FUNC()
    bool isVisible() const;

    SCRIPT_BIND_FUNC()
    void focus();

    // -- Engine side -----------------------------------------------------------
    [[nodiscard]] Rml::Element* resolve() const;

private:
    MUIWidget widget;
    SString   elementId;

    // Lookup cache, valid while the widget's DOM generation is unchanged.
    mutable Rml::Element* cachedElement    = nullptr;
    mutable uint32_t      cachedGeneration = 0;
    mutable uint32_t      cachedDomVersion = 0;
};

#endif // UI_ELEMENT_H
