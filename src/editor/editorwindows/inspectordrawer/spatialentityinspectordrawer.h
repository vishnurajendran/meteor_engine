//
// Created by Vishnu Rajendran on 2024-09-28.
//
#pragma once
#ifndef METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H
#define METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H

#include "core/utils/color.h"
#include "core/utils/glmhelper.h"
#include "imgui.h"
#include "inspectordrawer.h"

class MSpatialEntityInspectorDrawer : public MInspectorDrawer {
public:
    MSpatialEntityInspectorDrawer();
    SString getInspectorName() const override { return "Transform"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

    // Shared helpers available to all subclass drawers
    void drawColoredBoxOverLabel(const char* label, ImVec4 boxColor, float boxWidth);
    bool drawXYZComponent(const SString& label, SVector3& value);

    // Draws intensity + colour picker. Returns true if either value changed.
    // Callers pass their own float/SColor locals and apply them only when needed.
    bool drawLightIntensityAndColor(float& intensity, SColor& color);

private:
    bool drawTextField(const SString& label, SString& text);
    void drawTransformField(MSpatialEntity* target);
};

#endif //METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H