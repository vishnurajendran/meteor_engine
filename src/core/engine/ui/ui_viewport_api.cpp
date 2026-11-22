//
// ui_viewport_api.cpp
//

#include "ui_viewport_api.h"

#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/ui/viewport/viewport.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"

namespace
{
    MViewport* mainViewport()
    {
        auto* ui = MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>();
        return ui ? ui->getMainViewport() : nullptr;
    }
}

MViewport* SViewport::getMainViewport()
{
    return mainViewport();
}

bool SViewport::isUIConsumingMouse()
{
    const MViewport* viewport = mainViewport();
    return viewport && viewport->isConsumingMouse();
}

bool SViewport::isUIConsumingKeyboard()
{
    const MViewport* viewport = mainViewport();
    return viewport && viewport->isConsumingKeyboard();
}

int SViewport::getWidth()
{
    const MViewport* viewport = mainViewport();
    return viewport ? viewport->getWidth() : 0;
}

int SViewport::getHeight()
{
    const MViewport* viewport = mainViewport();
    return viewport ? viewport->getHeight() : 0;
}
