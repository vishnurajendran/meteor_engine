//
// Created by ssj5v on 01-05-2025.
//

#ifndef POINT_LIGHT_INSPECTOR_DRAWER_H
#define POINT_LIGHT_INSPECTOR_DRAWER_H
#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"


class MPointLightInspectorDrawer : MSpatialEntityInspectorDrawer {
public:
    void onDrawInspector(MSpatialEntity* target) override;
    bool canDraw(MSpatialEntity* entity) override;
private:
    void drawPLGui(MSpatialEntity* target);
    static bool registered;
};



#endif //POINT_LIGHT_INSPECTOR_DRAWER_H
