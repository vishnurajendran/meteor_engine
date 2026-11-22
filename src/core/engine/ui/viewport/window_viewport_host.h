//
// window_viewport_host.h
//
// The player's viewport host: the game window. Applies the cursor (SCursor)
// and the cursor shapes RmlUi asks for over elements.
//

#ifndef WINDOW_VIEWPORT_HOST_H
#define WINDOW_VIEWPORT_HOST_H

#include "viewport.h"

class MWindow;

class MWindowViewportHost : public IViewportHost
{
public:
    explicit MWindowViewportHost(MWindow* window) : window(window) {}

    void applyCursor(bool visible, ECursorState state) override;
    void setCursorShape(const SString& cursorName) override;

    // Once per frame, after the window's events and before the input
    // subsystem's poll(): while Locked and focused, warps the cursor to the
    // window's centre and tells the input subsystem.
    void update();

private:
    MWindow*     window      = nullptr;
    ECursorState cursorState = ECursorState::Free;
};

#endif // WINDOW_VIEWPORT_HOST_H
