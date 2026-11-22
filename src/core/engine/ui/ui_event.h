//
// ui_event.h
//
// SUIEvent - what a widget behaviour's event method receives:
//
//   <button id="play" onclick="onPlayClicked">Play</button>
//
//   onPlayClicked = function(self, event)
//       print(event.type, event.targetId)   -- "click", "play"
//       event:stopPropagation()
//   end,
//
// Any on<event> attribute works (onclick, onmouseover, onchange, onkeydown,
// ...). The value is the name of a method on the behaviour of the widget the
// element belongs to; on a <widget> tag it is the enclosing widget's method,
// since it is written in that widget's file.
//

#ifndef UI_EVENT_H
#define UI_EVENT_H

#include <cstdint>

#include "core/engine/input/input.h"
#include "core/engine/scripting/script_binding_macros.h"
#include "core/utils/sstring.h"

namespace Rml { class Event; }

SCRIPT_BIND_STRUCT()
struct SUIEvent
{
    // "click", "mouseover", "change", "keydown", ...
    SCRIPT_BIND_PROP()
    SString type;

    // Id of the element the event happened on (may be a child of the one
    // with the attribute).
    SCRIPT_BIND_PROP()
    SString targetId;

    // Id of the element whose attribute is being handled.
    SCRIPT_BIND_PROP()
    SString currentId;

    // Mouse position in viewport pixels, for mouse events.
    SCRIPT_BIND_PROP()
    float mouseX = 0.0f;

    SCRIPT_BIND_PROP()
    float mouseY = 0.0f;

    // Key events: the key.
    SCRIPT_BIND_PROP()
    EKeyCode key = EKeyCode::Unknown;

    // change / submit events: the new value of the form control.
    SCRIPT_BIND_PROP()
    SString value;

    // Stops the event reaching elements further up. Only works while the
    // handler is running.
    SCRIPT_BIND_FUNC()
    void stopPropagation() const;

    // -- Engine side -----------------------------------------------------------
    // The RmlUi event and the dispatch it belongs to; stopPropagation checks
    // the dispatch is still running, so a stored copy can't touch a dead event.
    Rml::Event* source = nullptr;
    uint64_t    dispatchSerial = 0;
};

#endif // UI_EVENT_H
