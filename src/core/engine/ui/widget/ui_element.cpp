//
// ui_element.cpp
//

#include "ui_element.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/StringUtilities.h>

#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"
#include "ui_widget_instance.h"

namespace
{
    MViewportSubsystem* ui()
    {
        return MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>();
    }

    void markChanged(const MUIWidget& widget)
    {
        auto* subsystem = ui();
        MViewport* viewport = subsystem ? subsystem->getViewport(widget.getViewportId()) : nullptr;
        if (viewport)
            viewport->markNeedsUpdate();
    }
}

Rml::Element* MUIElement::resolve() const
{
    const SUIWidgetInstance* owner = widget.resolve();
    if (!owner)
        return nullptr;

    // Found again whenever the widget's elements may have been replaced.
    if (cachedElement && cachedGeneration == owner->generation && cachedDomVersion == owner->domVersion)
        return cachedElement;

    cachedElement    = ui()->findElement(*owner, elementId);
    cachedGeneration = owner->generation;
    cachedDomVersion = owner->domVersion;
    return cachedElement;
}

bool MUIElement::isValid() const
{
    return resolve() != nullptr;
}

void MUIElement::setText(const SString& text)
{
    if (Rml::Element* element = resolve())
        ui()->replaceElementContent(widget, element, SString(Rml::StringUtilities::EncodeRml(text.str())));
}

SString MUIElement::getText() const
{
    const Rml::Element* element = resolve();
    return element ? SString(Rml::StringUtilities::DecodeRml(element->GetInnerRML())) : SString();
}

void MUIElement::setInnerRml(const SString& rml)
{
    if (Rml::Element* element = resolve())
        ui()->replaceElementContent(widget, element, rml);
}

void MUIElement::setClass(const SString& className, bool enabled)
{
    if (Rml::Element* element = resolve())
    {
        element->SetClass(className.str(), enabled);
        markChanged(widget);
    }
}

bool MUIElement::hasClass(const SString& className) const
{
    const Rml::Element* element = resolve();
    return element && element->IsClassSet(className.str());
}

bool MUIElement::setProperty(const SString& name, const SString& value)
{
    Rml::Element* element = resolve();
    if (!element)
        return false;
    const bool accepted = element->SetProperty(name.str(), value.str());
    markChanged(widget);
    return accepted;
}

void MUIElement::removeProperty(const SString& name)
{
    if (Rml::Element* element = resolve())
    {
        element->RemoveProperty(name.str());
        markChanged(widget);
    }
}

void MUIElement::setAttribute(const SString& name, const SString& value)
{
    if (Rml::Element* element = resolve())
    {
        element->SetAttribute(name.str(), value.str());
        markChanged(widget);
    }
}

SString MUIElement::getAttribute(const SString& name) const
{
    const Rml::Element* element = resolve();
    return element ? SString(element->GetAttribute<Rml::String>(name.str(), Rml::String())) : SString();
}

SString MUIElement::getValue() const
{
    const auto* control = dynamic_cast<const Rml::ElementFormControl*>(resolve());
    return control ? SString(control->GetValue()) : SString();
}

void MUIElement::setValue(const SString& value)
{
    if (auto* control = dynamic_cast<Rml::ElementFormControl*>(resolve()))
    {
        control->SetValue(value.str());
        markChanged(widget);
    }
}

void MUIElement::setVisible(bool visible)
{
    if (Rml::Element* element = resolve())
    {
        if (visible)
            element->RemoveProperty("display");
        else
            element->SetProperty("display", "none");
        markChanged(widget);
    }
}

bool MUIElement::isVisible() const
{
    const Rml::Element* element = resolve();
    return element && element->IsVisible();
}

void MUIElement::focus()
{
    if (Rml::Element* element = resolve())
        element->Focus();
}
