//
// Created by Vishnu Rajendran on 2024-09-28.
//

#include "inspectordrawer.h"
#include "spatialentityinspectordrawer.h"

std::map<const std::type_info*, MInspectorDrawer*>* MInspectorDrawer::drawers = nullptr;
MInspectorDrawer* MInspectorDrawer::defaultDrawer = nullptr;

void MInspectorDrawer::initialise() {
    drawers = new std::map<const std::type_info*, MInspectorDrawer*>();
    defaultDrawer = new MSpatialEntityInspectorDrawer();
}

void MInspectorDrawer::registerDrawer(MInspectorDrawer *drawer) {

    if(!drawer)
        return;

    if(drawers == nullptr){
        drawers = new std::map<const std::type_info*, MInspectorDrawer*>();
    }

    drawers->insert({&typeid(drawer), drawer});
}

MInspectorDrawer *MInspectorDrawer::getDrawer(MSpatialEntity *entity) {
    if(!entity)
        return nullptr;

    if(drawers->contains(&typeid(drawers))){
        auto key = &typeid(drawers);
        return (*drawers)[key];
    }

    return defaultDrawer;
}
