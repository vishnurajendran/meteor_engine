//
// Created by Vishnu Rajendran on 2024-09-28.
//
#pragma once
#ifndef METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H
#define METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H

#include <functional>
#include <typeindex>
#include <unordered_map>
#include "core/utils/color.h"
#include "core/utils/glmhelper.h"
#include "imgui.h"
#include "inspectordrawer.h"

#include "data/field_base.h"

class MSpatialEntityInspectorDrawer : public MInspectorDrawer {
    DEFINE_OBJECT_SUBCLASS(MSpatialEntityInspectorDrawer)
public:
    MSpatialEntityInspectorDrawer();
    SString getInspectorName() const override { return "Transform"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

    // Called by MInspectorDrawer::onDraw only when no custom drawer matched.
    // Iterates DECLARE_FIELD members and draws generic widgets.
    void drawDefaultFields(MSpatialEntity* target) override;

    // Shared helpers available to all subclass drawers
    void drawColoredBoxOverLabel(const char* label, ImVec4 boxColor, float boxWidth);
    bool drawXYZComponent(const SString& label, SVector3& value);
    bool drawXYComponent(const SString& label, SVector2& value);

    // Draws intensity + colour picker. Returns true if either value changed.
    // Callers pass their own float/SColor locals and apply them only when needed.
    bool drawLightIntensityAndColor(float& intensity, SColor& color);

private:
    bool drawTextField(const SString& label, SString& text);
    void drawTransformField(MSpatialEntity* target);

    // Iterates target->getFields() and draws a widget for each supported type.
    void drawFields(MSpatialEntity* target);

    // Per-type draw function. Receives the drawer (for access to helpers like
    // drawXYZComponent) and the field to draw. Returns true if value changed.
    using FieldDrawFn = std::function<bool(MSpatialEntityInspectorDrawer*, FieldBase*)>;
    static const std::unordered_map<std::type_index, FieldDrawFn> fieldDrawerMap;

    struct EulerCache
    {
        SQuaternion lastQuat { glm::identity<SQuaternion>() };
        SVector3    euler    { 0.0f };
    };
    std::unordered_map<MSpatialEntity*, EulerCache> eulerCache_;
};

#endif //METEOR_ENGINE_SPATIALENTITYINSPECTORDRAWER_H