//
// rml_event_listener.cpp
//

#include "rml_event_listener.h"

#include <cctype>

#include <RmlUi/Core/Element.h>

#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"
#include "core/utils/logger.h"

void MRmlEventListener::ProcessEvent(Rml::Event& event)
{
    if (auto* ui = MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>())
        ui->dispatchElementEvent(element, methodName, event);
}

void MRmlEventListener::OnDetach(Rml::Element*)
{
    delete this;
}

Rml::EventListener* MRmlEventListenerInstancer::InstanceEventListener(const Rml::String& value, Rml::Element* element)
{
    // "onPauseClicked", also accepted as "onPauseClicked()".
    std::string name = SString(value).trimmed().str();
    if (name.size() > 2 && name.compare(name.size() - 2, 2, "()") == 0)
        name.resize(name.size() - 2);

    bool identifier = !name.empty() && (std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_');
    for (char c : name)
        identifier = identifier && (std::isalnum(static_cast<unsigned char>(c)) || c == '_');

    if (!identifier)
    {
        MERROR(SString::format("UI: event attribute value '{0}' on <{1} id=\"{2}\"> must be the name of a method on "
                               "the widget's behaviour, e.g. onclick=\"onPlayClicked\"",
                               SString(value), SString(element ? element->GetTagName() : Rml::String("?")),
                               SString(element ? element->GetId() : Rml::String())));
        return nullptr;
    }
    return new MRmlEventListener(SString(name), element);
}
