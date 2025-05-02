//
// Created by ssj5v on 01-05-2025.
//

#ifndef SPOT_LIGHT_INSPECTOR_DRAWER_H
#define SPOT_LIGHT_INSPECTOR_DRAWER_H
#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"


class MSpotLightInspectorDrawer : MSpatialEntityInspectorDrawer {
public:
    void onDrawInspector(MSpatialEntity* target) override;
    bool canDraw(MSpatialEntity* entity) override;
private:
    void drawSLGui(MSpatialEntity* target);
    static bool registered;
};



#endif //SPOT_LIGHT_INSPECTOR_DRAWER_H
