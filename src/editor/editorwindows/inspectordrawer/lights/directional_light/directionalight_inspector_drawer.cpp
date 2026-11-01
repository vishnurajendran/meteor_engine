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

        START_FIELD_DRAW("Cascades", "Number of shadow maps the shadow distance is split into.\nMore = sharper shadows near the camera, one extra shadow pass each.")
            int count = light->getCascadeCount();
            if (ImGui::SliderInt("##v", &count, 1, MDirectionalLight::MAX_CASCADES, "%d", ImGuiSliderFlags_AlwaysClamp))
                light->setCascadeCount(count);
        END_FIELD_DRAW()

        START_FIELD_DRAW("Cascade Split", "How cascade ranges are spread.\n0 = equal lengths, 1 = logarithmic (most detail near the camera).")
            float split = light->getCascadeSplit();
            if (MInspectorField::SliderFloat("##v", &split, 0.0f, 1.0f, "%.2f"))
                light->setCascadeSplit(split);
        END_FIELD_DRAW()

        START_FIELD_DRAW("Show Cascades", "Debug: tint the scene by cascade (red, green, blue, yellow). Not saved.")
            bool show = light->getShowCascades();
            if (ImGui::Checkbox("##v", &show))
                light->setShowCascades(show);
        END_FIELD_DRAW()
    }
}