#pragma once
#ifndef POST_PROCESS_STACK_INSPECTOR_DRAWER_H
#define POST_PROCESS_STACK_INSPECTOR_DRAWER_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class SPostProcessEffectSettings;

// Inspector for MPostProcessStackEntity: lists the stack's effects with
// their settings (drawn generically from DECLARE_FIELD members), plus
// Add Effect / Remove. Edits go straight to the pipeline's stack.
class MPostProcessStackInspectorDrawer : public MSpatialEntityInspectorDrawer {
    DEFINE_OBJECT_SUBCLASS(MPostProcessStackInspectorDrawer)
public:
    SString getInspectorName() const override { return "Post Process Stack"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    static void drawSettingsFields(SPostProcessEffectSettings* settings);
    static const bool registered;
};

#endif
