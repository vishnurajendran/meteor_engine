#include "directionalight_inspector_drawer.h"
#include "core/engine/lighting/directional/directional_light.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/memory/mnew.h"

const bool MDirecionalLightInspectorDrawer::registered = []() {
    registerDrawer(mnew<MDirecionalLightInspectorDrawer>());
    return true;
}();

void MDirecionalLightInspectorDrawer::onDrawInspector(MSpatialEntity* target) {
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    drawDLGui(dynamic_cast<MDirectionalLight*>(target));
}

bool MDirecionalLightInspectorDrawer::canDraw(MSpatialEntity* entity) {
    return dynamic_cast<MDirectionalLight*>(entity) != nullptr;
}

void MDirecionalLightInspectorDrawer::drawDLGui(MDirectionalLight* light) {
    if (!light) return;

    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        float  intensity = light->getIntensity();
        SColor color     = light->getColor();
        if (drawLightIntensityAndColor(intensity, color)) {
            light->setIntensity(intensity);
            light->setColor(color);
        }

        // Shadow settings - per-light, not global.

        START_FIELD_DRAW("Cast Shadow")
            bool casts = light->getCastsShadow();
            if (ImGui::Checkbox("##v", &casts))
                light->setCastsShadow(casts);
        END_FIELD_DRAW()

        START_FIELD_DRAW("Smooth Shadows")
            bool smooth = light->getSmoothShadow();
            if (ImGui::Checkbox("##v", &smooth))
                light->setSmoothShadow(smooth);
        END_FIELD_DRAW()

        START_FIELD_DRAW("Shadow Distance")
            float dist = light->getShadowDistance();
            if (MInspectorField::DragFloat("##v", &dist, 1.0f,
                                 MDirectionalLight::MIN_SHADOW_DISTANCE,
                                 MDirectionalLight::MAX_SHADOW_DISTANCE, "%.0f",
                                 ImGuiSliderFlags_AlwaysClamp))
                light->setShadowDistance(dist);
        END_FIELD_DRAW()
    }
}