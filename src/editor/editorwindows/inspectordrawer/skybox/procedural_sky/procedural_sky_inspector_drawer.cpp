//
// MProceduralSkyInspectorDrawer
//

#include <imgui.h>
#include "procedural_sky_inspector_drawer.h"
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

    // Draw the base transform controls first (position / rotation / scale).
    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    if (!ImGui::CollapsingHeader("Procedural Skybox"))
        return;

    constexpr float LABEL_COL_W = 150.0f;
    constexpr ImGuiTableFlags TABLE_FLAGS = ImGuiTableFlags_None;

    // ── Sun ──────────────────────────────────────────────────────────────────
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

            // Back-solve elevation from the current sun direction Y component.
            SVector3 dir      = sky->getSunDirection();
            float    elevation = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
            float    azimuth   = glm::degrees(std::atan2(dir.x, dir.z));

            if (ImGui::SliderFloat("##Elevation", &elevation, -90.0f, 90.0f, "%.1f deg"))
                sky->setSunAngles(elevation, azimuth);
        }

        // Azimuth
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Azimuth");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            SVector3 dir     = sky->getSunDirection();
            float    elevation = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
            float    azimuth   = glm::degrees(std::atan2(dir.x, dir.z));

            if (ImGui::SliderFloat("##Azimuth", &azimuth, -180.0f, 180.0f, "%.1f deg"))
                sky->setSunAngles(elevation, azimuth);
        }

        // Day/night cycle speed
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Cycle Speed");
            ImGui::SetItemTooltip("Degrees per second. 0 = static sun.");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            float speed = sky->getDayNightCycleSpeed();
            if (ImGui::DragFloat("##CycleSpeed", &speed, 0.5f, -360.0f, 360.0f, "%.1f deg/s"))
                sky->setDayNightCycleSpeed(speed);
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

        // Sun size convergence
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

    // ── Atmosphere ───────────────────────────────────────────────────────────
    ImGui::SeparatorText("Atmosphere");

    if (ImGui::BeginTable("##sky_atmo", 2, TABLE_FLAGS))
    {
        ImGui::TableSetupColumn("label",  ImGuiTableColumnFlags_WidthFixed,   LABEL_COL_W);
        ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

        // Atmosphere thickness
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Thickness");
            ImGui::SetItemTooltip("Air density. 0 = space, 5 = very thick haze.");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            float v = sky->getAtmosphereThickness();
            if (ImGui::SliderFloat("##AtmoThick", &v, 0.0f, 5.0f, "%.2f"))
                sky->setAtmosphereThickness(v);
        }

        // Sky tint
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Sky Tint");
            ImGui::SetItemTooltip("Colour multiplier over the scattering result.\nNeutral = (0.5, 0.5, 0.5).");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            SVector3 tint = sky->getSkyTint();
            if (ImGui::ColorEdit3("##SkyTint", &tint.x))
                sky->setSkyTint(tint);
        }

        // Ground color
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

    // ── Rendering ────────────────────────────────────────────────────────────
    ImGui::SeparatorText("Rendering");

    if (ImGui::BeginTable("##sky_render", 2, TABLE_FLAGS))
    {
        ImGui::TableSetupColumn("label",  ImGuiTableColumnFlags_WidthFixed,   LABEL_COL_W);
        ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

        // Exposure
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