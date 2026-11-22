//
// ui_event.cpp
//

#include "ui_event.h"

#include <RmlUi/Core/Event.h>

#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"

void SUIEvent::stopPropagation() const
{
    // Only while this event's handler runs: a copy kept by a script must not
    // reach an event RmlUi has finished with.
    const auto* ui = MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>();
    if (ui && source && dispatchSerial != 0 && dispatchSerial == ui->getCurrentDispatchSerial())
        source->StopPropagation();
}
