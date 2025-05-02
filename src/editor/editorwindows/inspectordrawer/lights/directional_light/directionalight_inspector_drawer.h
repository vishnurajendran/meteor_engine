//
// Created by ssj5v on 24-04-2025.
//

#ifndef DIRECTIONALIGHT_INSPECTOR_DRAWER_H
#define DIRECTIONALIGHT_INSPECTOR_DRAWER_H
#include "editor/editorwindows/inspectordrawer/inspectordrawer.h"
#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"


class MDirectionalLight;
class MDirecionalLightInspectorDrawer : MSpatialEntityInspectorDrawer
{
private:
    static const bool registered;

public:
    SString getInspectorName() const override { return "Directional Light"; }
    virtual bool canDraw(MSpatialEntity* entity) override;
protected:
    virtual void onDrawInspector(MSpatialEntity* target) override;
private:
    void drawDLGui(MDirectionalLight* light);
};


#endif //DIRECTIONALIGHT_INSPECTOR_DRAWER_H
