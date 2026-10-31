//
// MProceduralSkyInspectorDrawer
//

#include "procedural_sky_inspector_drawer.h"
#include <imgui.h>
#include "core/engine/lighting/ambient/ambient_light.h"
#include "core/engine/lighting/directional/directional_light.h"
#include "core/engine/skybox/procedural_sky/procedural_sky.h"
#include "core/memory/mnew.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"

bool MProceduralSkyInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MProceduralSkyInspectorDrawer>());
    return true;
}();

bool MProceduralSkyInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MProceduralSkyboxEntity*>(entity) != nullptr;
}

void MProceduralSkyInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    auto* sky = dynamic_cast<MProceduralSkyboxEntity*>(target);

    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    if (!ImGui::CollapsingHeader("Procedural Skybox"))
        return;

    // ---- Detect child overrides --------------------------------------------
    bool hasDirLight    = false;
    bool hasAmbLight    = false;
    for (auto* child : sky->getChildren())
    {
        if (dynamic_cast<MDirectionalLight*>(child))   hasDirLight  = true;
        if (dynamic_cast<MAmbientLightEntity*>(child)) hasAmbLight  = true;
    }

    if (hasDirLight)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.9f, 0.4f, 1.0f));
        ImGui::TextUnformatted("Sun driven by child Directional Light");
        ImGui::PopStyleColor();
        ImGui::TextDisabled("Elevation, Azimuth and Cycle Speed are inactive.");
        ImGui::Spacing();
    }
    if (hasAmbLight)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.9f, 0.4f, 1.0f));
        ImGui::TextUnformatted("Ambient driven by child Ambient Light");
        ImGui::PopStyleColor();
        ImGui::TextDisabled("Ambient color and intensity are set from the sky each frame.");
        ImGui::Spacing();
    }

    // ---- Sun ---------------------------------------------------------------
    ImGui::SeparatorText("Sun");

    {
        SVector3 dir       = sky->getSunDirection();
        float    elevation = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
        float    azimuth   = glm::degrees(std::atan2(dir.x, dir.z));

        ImGui::BeginDisabled(hasDirLight);

        START_FIELD_DRAW("Elevation")
            if (MInspectorField::SliderFloat("##v", &elevation, -90.0f, 90.0f, "%.1f deg"))
                sky->setSunAngles(elevation, azimuth);
        END_FIELD_DRAW()

        START_FIELD_DRAW("Azimuth")
            if (MInspectorField::SliderFloat("##v", &azimuth, -180.0f, 180.0f, "%.1f deg"))
                sky->setSunAngles(elevation, azimuth);
        END_FIELD_DRAW()

        START_FIELD_DRAW("Cycle Speed",
                         "Cycles per second. 1 cycle = full 360 degree pass.\n"
                         "0 = static. Ignored when a Directional Light child is present.")
            float speed = sky->getDayNightCycleSpeed();
            if (MInspectorField::DragFloat("##v", &speed, 0.001f, -10.0f, 10.0f, "%.4f cyc/s"))
                sky->setDayNightCycleSpeed(speed);
        END_FIELD_DRAW()

        ImGui::EndDisabled();
    }

    START_FIELD_DRAW("Sun Size")
        float v = sky->getSunSize();
        if (MInspectorField::SliderFloat("##v", &v, 0.0f, 0.2f, "%.3f"))
            sky->setSunSize(v);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Sun Sharpness", "Higher = harder disc edge.")
        float v = sky->getSunSizeConvergence();
        if (MInspectorField::SliderFloat("##v", &v, 1.0f, 10.0f, "%.1f"))
            sky->setSunSizeConvergence(v);
    END_FIELD_DRAW()

    // ---- Atmosphere --------------------------------------------------------
    ImGui::SeparatorText("Atmosphere");

    START_FIELD_DRAW("Thickness", "Air density. 0 = space, 5 = thick haze.")
        float v = sky->getAtmosphereThickness();
        if (MInspectorField::SliderFloat("##v", &v, 0.0f, 5.0f, "%.2f"))
            sky->setAtmosphereThickness(v);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Sky Tint", "Neutral = (0.5, 0.5, 0.5).")
        SVector3 tint = sky->getSkyTint();
        if (ImGui::ColorEdit3("##v", &tint.x))
            sky->setSkyTint(tint);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Ground Color")
        SVector3 col = sky->getGroundColor();
        if (ImGui::ColorEdit3("##v", &col.x))
            sky->setGroundColor(col);
    END_FIELD_DRAW()

    // ---- Rendering ---------------------------------------------------------
    ImGui::SeparatorText("Rendering");

    START_FIELD_DRAW("Exposure", "Linear brightness scale before tone-mapping.")
        float v = sky->getExposure();
        if (MInspectorField::SliderFloat("##v", &v, 0.0f, 8.0f, "%.2f"))
            sky->setExposure(v);
    END_FIELD_DRAW()
}