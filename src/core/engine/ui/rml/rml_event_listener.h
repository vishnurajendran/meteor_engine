//
// rml_event_listener.h
//
// Replaces RmlUi's inline-script handling of on<event> attributes:
//
//   <button onclick="onPlayClicked">
//
// The attribute value is a method name on the behaviour of the widget the
// element belongs to. No Lua closures are stored on the C++ side, so nothing
// outlives the script domain that created it.
//

#ifndef RML_EVENT_LISTENER_H
#define RML_EVENT_LISTENER_H

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>

#include "core/utils/sstring.h"

class MRmlEventListener : public Rml::EventListener
{
public:
    MRmlEventListener(const SString& methodName, Rml::Element* element)
        : methodName(methodName), element(element) {}

    void ProcessEvent(Rml::Event& event) override;
    // One listener per attribute; RmlUi detaches it when the attribute or the
    // element goes away.
    void OnDetach(Rml::Element* element) override;

private:
    SString       methodName;
    Rml::Element* element = nullptr;   // the element carrying the attribute
};

class MRmlEventListenerInstancer : public Rml::EventListenerInstancer
{
public:
    Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override;
};

#endif // RML_EVENT_LISTENER_H
