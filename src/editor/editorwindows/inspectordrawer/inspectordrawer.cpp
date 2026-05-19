//
// Created by Vishnu Rajendran on 2024-09-28.
//

#include "inspectordrawer.h"
#include "spatialentityinspectordrawer.h"

std::vector<MInspectorDrawer*> MInspectorDrawer::drawers;
MInspectorDrawer*              MInspectorDrawer::defaultDrawer = nullptr;

void MInspectorDrawer::initialise() {
    defaultDrawer = new MSpatialEntityInspectorDrawer();
}

void MInspectorDrawer::registerDrawer(MInspectorDrawer* drawer) {
    if (!drawer)
        return;
    drawers.push_back(drawer);
}

void MInspectorDrawer::onDraw(MSpatialEntity* target) {
    onDrawInspector(target);

    // When the default drawer is handling the entity (no custom drawer matched),
    // run the generic field-based fallback so DECLARE_FIELD members get widgets
    // automatically. Custom drawers skip this -- they draw their own controls.
    if (this == defaultDrawer)
        drawDefaultFields(target);
}

MInspectorDrawer* MInspectorDrawer::getDrawer(MSpatialEntity* entity) {
    if (!entity)
        return nullptr;

    for (auto* drawer : drawers) {
        if (drawer->canDraw(entity))
            return drawer;
    }
    return defaultDrawer;
}