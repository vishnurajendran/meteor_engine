//
// Created by ssj5v on 24-04-2025.
//

#ifndef AMBIENTLIGHT_INSPECTOR_DRAWER_H
#define AMBIENTLIGHT_INSPECTOR_DRAWER_H
#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"


class MAmbientLightEntity;
class MAmbientLightInspectorDrawer : MSpatialEntityInspectorDrawer {
private:
    static const bool registered;

public:
    virtual void onDrawInspector(MSpatialEntity* target) override;
    virtual bool canDraw(MSpatialEntity* entity) override;

private:
    void drawALGui(MAmbientLightEntity* light);
};



#endif //AMBIENTLIGHT_INSPECTOR_DRAWER_H
