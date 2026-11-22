//
// cursor.cpp
//
// The cursor belongs to the main viewport, whose host (the player's window,
// the editor's Scene view) applies it.
//

#include "cursor.h"

#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/ui/viewport/viewport.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"

static MViewport* mainViewport()
{
    auto* ui = MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>();
    return ui ? ui->getMainViewport() : nullptr;
}

void SCursor::setCursorVisible(bool visible)
{
    if (MViewport* viewport = mainViewport())
        viewport->setCursorVisible(visible);
}

bool SCursor::isCursorVisible()
{
    const MViewport* viewport = mainViewport();
    return !viewport || viewport->isCursorVisible();
}

void SCursor::setCursorState(ECursorState state)
{
    if (MViewport* viewport = mainViewport())
        viewport->setCursorState(state);
}

ECursorState SCursor::getCursorState()
{
    const MViewport* viewport = mainViewport();
    return viewport ? viewport->getCursorState() : ECursorState::Free;
}
