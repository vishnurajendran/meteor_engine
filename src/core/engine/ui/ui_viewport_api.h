//
// ui_viewport_api.h
//
// SViewport - script access to the main viewport (the game window in a
// player, the Scene view while playing in the editor).
//
//   local viewport = SViewport.getMainViewport()   -- MViewport: addWidget, getCurrentWidget, clear
//   if SViewport.isUIConsumingMouse() then return end
//
// The consumed queries are also on SInput, next to the key queries. The
// cursor is SCursor (core/engine/input/cursor.h).
//

#ifndef UI_VIEWPORT_API_H
#define UI_VIEWPORT_API_H

#include "core/engine/scripting/script_binding_macros.h"
#include "core/utils/sstring.h"
#include "viewport/viewport.h"

SCRIPT_BIND_STRUCT()
struct SViewport
{
    // The game's viewport (the window in a player, the Scene view's game
    // image in the editor). nil before the UI has started.
    SCRIPT_BIND_FUNC_STATIC()
    static MViewport* getMainViewport();

    // True while the mouse is over a UI element (not just a widget's
    // transparent background) or dragging one.
    SCRIPT_BIND_FUNC_STATIC()
    static bool isUIConsumingMouse();

    // True while a text field has keyboard focus.
    SCRIPT_BIND_FUNC_STATIC()
    static bool isUIConsumingKeyboard();

    // Size in pixels.
    SCRIPT_BIND_FUNC_STATIC()
    static int getWidth();

    SCRIPT_BIND_FUNC_STATIC()
    static int getHeight();
};

#endif // UI_VIEWPORT_API_H
