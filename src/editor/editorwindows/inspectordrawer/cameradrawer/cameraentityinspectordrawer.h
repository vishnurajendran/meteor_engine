//
// Created by ssj5v on 13-10-2024.
//

#ifndef CAMERAENTITYINSPECTORDRAWER_H
#define CAMERAENTITYINSPECTORDRAWER_H
#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"


class MCameraEntity;

class MCameraEntityInspectorDrawer : MSpatialEntityInspectorDrawer {
private:
    static const bool registered;
public:
    virtual void onDrawInspector(MSpatialEntity *target) override;
    virtual bool canDraw(MSpatialEntity *entity) override;
private:
    void drawCameraGUI(MCameraEntity* cameraTarget);
};



#endif //CAMERAENTITYINSPECTORDRAWER_H
