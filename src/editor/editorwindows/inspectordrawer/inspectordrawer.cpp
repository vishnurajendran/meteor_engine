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

// Delegates straight to the virtual; the unused getName() call has been removed.
void MInspectorDrawer::onDraw(MSpatialEntity* target) {
    onDrawInspector(target);
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