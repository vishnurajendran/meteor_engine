//
// MProceduralSkyInspectorDrawer
//

#include "procedural_sky_inspector_drawer.h"
#include <imgui.h>
#include "core/engine/lighting/ambient/ambient_light.h"
#include "core/engine/lighting/directional/directional_light.h"
#include "core/engine/skybox/procedural_sky/procedural_sky.h"

bool MProceduralSkyInspectorDrawer::registered = []()
{
    registerDrawer(new MProceduralSkyInspectorDrawer());
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

    constexpr float           LABEL_COL_W  = 150.0f;
    constexpr ImGuiTableFlags TABLE_FLAGS  = ImGuiTableFlags_None;

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

    if (ImGui::BeginTable("##sky_sun", 2, TABLE_FLAGS))
    {
        ImGui::TableSetupColumn("label",  ImGuiTableColumnFlags_WidthFixed,   LABEL_COL_W);
        ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

        // Elevation
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Elevation");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            SVector3 dir       = sky->getSunDirection();
            float    elevation = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
            float    azimuth   = glm::degrees(std::atan2(dir.x, dir.z));

            if (hasDirLight) ImGui::BeginDisabled();
            if (ImGui::SliderFloat("##Elevation", &elevation, -90.0f, 90.0f, "%.1f deg"))
                sky->setSunAngles(elevation, azimuth);
            if (hasDirLight) ImGui::EndDisabled();
        }

        // Azimuth
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Azimuth");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            SVector3 dir       = sky->getSunDirection();
            float    elevation = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
            float    azimuth   = glm::degrees(std::atan2(dir.x, dir.z));

            if (hasDirLight) ImGui::BeginDisabled();
            if (ImGui::SliderFloat("##Azimuth", &azimuth, -180.0f, 180.0f, "%.1f deg"))
                sky->setSunAngles(elevation, azimuth);
            if (hasDirLight) ImGui::EndDisabled();
        }

        // Cycle speed
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Cycle Speed");
            ImGui::SetItemTooltip(
                "Cycles per second. 1 cycle = full 360 degree pass.\n"
                "0 = static. Ignored when a Directional Light child is present.");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            float speed = sky->getDayNightCycleSpeed();
            if (hasDirLight) ImGui::BeginDisabled();
            if (ImGui::DragFloat("##CycleSpeed", &speed, 0.001f, -10.0f, 10.0f, "%.4f cyc/s"))
                sky->setDayNightCycleSpeed(speed);
            if (hasDirLight) ImGui::EndDisabled();
        }

        // Sun size
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Sun Size");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            float v = sky->getSunSize();
            if (ImGui::SliderFloat("##SunSize", &v, 0.0f, 0.2f, "%.3f"))
                sky->setSunSize(v);
        }

        // Sun sharpness
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Sun Sharpness");
            ImGui::SetItemTooltip("Higher = harder disc edge.");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            float v = sky->getSunSizeConvergence();
            if (ImGui::SliderFloat("##SunConv", &v, 1.0f, 10.0f, "%.1f"))
                sky->setSunSizeConvergence(v);
        }

        ImGui::EndTable();
    }

    // ---- Atmosphere --------------------------------------------------------
    ImGui::SeparatorText("Atmosphere");

    if (ImGui::BeginTable("##sky_atmo", 2, TABLE_FLAGS))
    {
        ImGui::TableSetupColumn("label",  ImGuiTableColumnFlags_WidthFixed,   LABEL_COL_W);
        ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Thickness");
            ImGui::SetItemTooltip("Air density. 0 = space, 5 = thick haze.");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            float v = sky->getAtmosphereThickness();
            if (ImGui::SliderFloat("##AtmoThick", &v, 0.0f, 5.0f, "%.2f"))
                sky->setAtmosphereThickness(v);
        }

        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Sky Tint");
            ImGui::SetItemTooltip("Neutral = (0.5, 0.5, 0.5).");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            SVector3 tint = sky->getSkyTint();
            if (ImGui::ColorEdit3("##SkyTint", &tint.x))
                sky->setSkyTint(tint);
        }

        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Ground Color");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            SVector3 col = sky->getGroundColor();
            if (ImGui::ColorEdit3("##GroundCol", &col.x))
                sky->setGroundColor(col);
        }

        ImGui::EndTable();
    }

    // ---- Rendering ---------------------------------------------------------
    ImGui::SeparatorText("Rendering");

    if (ImGui::BeginTable("##sky_render", 2, TABLE_FLAGS))
    {
        ImGui::TableSetupColumn("label",  ImGuiTableColumnFlags_WidthFixed,   LABEL_COL_W);
        ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Exposure");
            ImGui::SetItemTooltip("Linear brightness scale before tone-mapping.");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            float v = sky->getExposure();
            if (ImGui::SliderFloat("##Exposure", &v, 0.0f, 8.0f, "%.2f"))
                sky->setExposure(v);
        }

        ImGui::EndTable();
    }
}