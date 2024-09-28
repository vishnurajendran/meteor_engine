//
// Created by Vishnu Rajendran on 2024-09-28.
//

#ifndef METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H
#define METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H

#include "inspectordrawer.h"
#include "imgui.h"
#include "core/utils/glmhelper.h"

class MSpatialEntityInspectorDrawer : public MInspectorDrawer {
    void onDrawInspector(MSpatialEntity *target) override;
    void drawColoredBoxOverLabel(const char* label, ImVec4 boxColor, float boxWidth);
    bool drawXYZComponent(const SString& label, SVector3& value);
};

#endif //METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H
