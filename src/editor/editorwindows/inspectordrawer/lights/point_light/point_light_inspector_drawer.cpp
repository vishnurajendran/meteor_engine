#include "point_light_inspector_drawer.h"
#include "core/engine/lighting/dynamiclights/point_light/point_light.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/memory/mnew.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"

bool MPointLightInspectorDrawer::registered = []() {
    registerDrawer(mnew<MPointLightInspectorDrawer>());
    return true;
}();

void MPointLightInspectorDrawer::onDrawInspector(MSpatialEntity* target) {
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    drawPLGui(target);
}

bool MPointLightInspectorDrawer::canDraw(MSpatialEntity* entity) {
    return dynamic_cast<MPointLight*>(entity) != nullptr;
}

void MPointLightInspectorDrawer::drawPLGui(MSpatialEntity* target) {
    auto* light = dynamic_cast<MPointLight*>(target);
    if (!light) return;

    if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        float  range       = light->getRange();
        float  intensity   = light->getIntensity();
        SColor color       = light->getColor();
        bool   castsShadow = light->getCastsShadow();
        bool   changed     = false;

        START_FIELD_DRAW("Range")
            if (MInspectorField::DragFloat("##v", &range, 0.01f, 0.0f, FLT_MAX, "%.2f"))
                changed = true;
        END_FIELD_DRAW()

        START_FIELD_DRAW("Cast Shadow")
            if (ImGui::Checkbox("##v", &castsShadow))
                light->setCastsShadow(castsShadow);
        END_FIELD_DRAW()

        START_FIELD_DRAW("Smooth Shadows")
            bool smoothShadow = light->getSmoothShadow();
            if (ImGui::Checkbox("##v", &smoothShadow))
                light->setSmoothShadow(smoothShadow);
        END_FIELD_DRAW()

        if (drawLightIntensityAndColor(intensity, color))
            changed = true;

        if (changed) {
            light->setRange(glm::max(range, 0.0f));
            light->setIntensity(glm::max(intensity, 0.0f));
            light->setColor(color);
        }
    }
}