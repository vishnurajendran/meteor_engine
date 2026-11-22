//
// cursor.h
//
// SCursor - the cursor over the game viewport, for gameplay code and scripts.
// Visibility and locking are separate:
//
//   SCursor.setCursorState(ECursorState.Locked)   -- mouse-look: hidden, held at the centre
//   SCursor.setCursorVisible(false)               -- hide it, but let it move freely
//
// Both reset to visible / Free when Play stops.
//

#ifndef SCURSOR_H
#define SCURSOR_H

#include "core/engine/scripting/script_binding_macros.h"

SCRIPT_BIND_ENUM()
enum class ECursorState : int
{
    // Moves freely and can leave the window.
    Free     = 0,
    // Kept inside the window. (In the editor, the same as Free: the Scene
    // view can't confine the OS cursor.)
    Confined = 1,
    // Hidden whatever setCursorVisible says, and held at the viewport's centre.
    // SInput.getMouseDelta() keeps reporting movement (raw mouse motion) and
    // getMousePos() reads the centre. In the editor, Escape unlocks and shows
    // the cursor (the state becomes Free, visible).
    Locked   = 2,
};

SCRIPT_BIND_STRUCT()
struct SCursor
{
    // Show or hide the cursor over the viewport. Locked hides it regardless.
    SCRIPT_BIND_FUNC_STATIC()
    static void setCursorVisible(bool visible);

    SCRIPT_BIND_FUNC_STATIC()
    static bool isCursorVisible();

    SCRIPT_BIND_FUNC_STATIC()
    static void setCursorState(ECursorState state);

    SCRIPT_BIND_FUNC_STATIC()
    static ECursorState getCursorState();
};

#endif // SCURSOR_H
