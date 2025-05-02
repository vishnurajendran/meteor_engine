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
    SString getInspectorName() const override {return "Camera";}
    virtual bool canDraw(MSpatialEntity *entity) override;
private:
    virtual void onDrawInspector(MSpatialEntity *target) override;
    void drawCameraGUI(MCameraEntity* cameraTarget);
};



#endif //CAMERAENTITYINSPECTORDRAWER_H
