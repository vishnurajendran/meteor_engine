//
// editorthemes.cpp
//
// Suggested location: src/editor/window/imgui/editorthemes.cpp
//

#include "editor_themes.h"
#include "core/engine/engine_statics.h"
#include "core/utils/logger.h"
#include "editor/settings/editor_settings.h"
#include "imgui.h"

const std::map<std::string, std::function<void()>>& MEditorThemes::getThemeMap()
{
    static const std::map<std::string, std::function<void()>> map =
    {
        { "me::midnight", applyMidnightTheme },
        { "me::dark",     applyDarkTheme     },
    };
    return map;
}

void MEditorThemes::applyTheme()
{
    auto* settings = dynamic_cast<MEditorSettings*>(MEngineStatics::getEngineSettings());
    const std::string& id = settings->editorTheme.get();

    const auto& map = getThemeMap();
    auto it = map.find(id);
    if (it != map.end())
    {
        it->second();
        MVERBOSE(SString::format("MEditorThemes:: Applied theme \"{0}\"", id));
    }
    else
    {
        MWARN(SString::format(
            "MEditorThemes:: Unknown theme \"{0}\" - falling back to me::midnight", id));
        applyMidnightTheme();
    }
}

// Rounding, padding, spacing - independent of colour so every theme feels
// structurally identical. Called at the end of each theme function.

void MEditorThemes::applyCommonStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding      = ImVec2(8.00f, 8.00f);
    style.FramePadding       = ImVec2(5.00f, 2.00f);
    style.CellPadding        = ImVec2(6.00f, 6.00f);
    style.ItemSpacing        = ImVec2(6.00f, 6.00f);
    style.ItemInnerSpacing   = ImVec2(6.00f, 6.00f);
    style.TouchExtraPadding  = ImVec2(0.00f, 0.00f);
    style.IndentSpacing      = 25;
    style.ScrollbarSize      = 15;
    style.GrabMinSize        = 10;
    style.WindowBorderSize   = 1;
    style.ChildBorderSize    = 1;
    style.PopupBorderSize    = 1;
    style.FrameBorderSize    = 1;
    style.TabBorderSize      = 1;
    style.WindowRounding     = 7;
    style.ChildRounding      = 4;
    style.FrameRounding      = 3;
    style.PopupRounding      = 4;
    style.ScrollbarRounding  = 9;
    style.GrabRounding       = 3;
    style.LogSliderDeadzone  = 4;
    style.TabRounding        = 4;
}

// ═══════════════════════════════════════════════════════════════════════════════
// MIDNIGHT - graphite greys with a single blue accent
// (originally ported from applyDeepDarkTheme - reworked for contrast and consistency)
// ═══════════════════════════════════════════════════════════════════════════════

void MEditorThemes::applyMidnightTheme()
{
    // ── Palette ─────────────────────────────────────────────────────────────
    // Layers go dark → light: recessed (inputs) → base (title bars / tab strip)
    // → window → raised (menus / popups) → control (buttons).
    // Every "selected / active" state uses the same accent - that consistency
    // is most of what makes an editor read as one coherent tool.
    const ImVec4 bgRecessed   = ImVec4(0.051f, 0.055f, 0.059f, 1.00f); // #0D0E0F - input fields
    const ImVec4 bgBase       = ImVec4(0.063f, 0.067f, 0.071f, 1.00f); // #101112 - title bars, tab strip, scrollbar track
    const ImVec4 bgWindow     = ImVec4(0.086f, 0.090f, 0.098f, 1.00f); // #16171A - panel background
    const ImVec4 bgRaised     = ImVec4(0.114f, 0.118f, 0.129f, 1.00f); // #1D1E21 - menu bar, popups, table headers
    const ImVec4 control      = ImVec4(0.153f, 0.157f, 0.169f, 1.00f); // #27282B - buttons
    const ImVec4 controlHover = ImVec4(0.204f, 0.208f, 0.224f, 1.00f); // #343539
    const ImVec4 controlLight = ImVec4(0.282f, 0.286f, 0.306f, 1.00f); // #48494E
    const ImVec4 border       = ImVec4(0.161f, 0.165f, 0.180f, 1.00f); // #292A2E

    // Same blue as the hierarchy's selected-row colour (COL_SEL_HOVER) - so the
    // theme and the hand-coloured hierarchy rows match without touching that file.
    const ImVec4 accent       = ImVec4(0.216f, 0.431f, 0.902f, 1.00f); // #376EE6
    const ImVec4 accentLight  = ImVec4(0.330f, 0.560f, 0.960f, 1.00f); // #548FF5 - hover / small marks on dark bg

    const ImVec4 text         = ImVec4(0.900f, 0.910f, 0.930f, 1.00f); // off-white - less glare than pure white
    const ImVec4 textDisabled = ImVec4(0.480f, 0.490f, 0.520f, 1.00f);

    // Same colour, different alpha - keeps the hue identical across states.
    auto withAlpha = [](ImVec4 c, float a) { c.w = a; return c; };

    ImVec4* colors = ImGui::GetStyle().Colors;

    // Text
    colors[ImGuiCol_Text]                  = text;
    colors[ImGuiCol_TextDisabled]          = textDisabled;
    colors[ImGuiCol_TextSelectedBg]        = withAlpha(accent, 0.35f);
    colors[ImGuiCol_TextLink]              = accentLight;

    // Windows - all opaque now, so a panel looks the same wherever it is docked
    colors[ImGuiCol_WindowBg]              = bgWindow;
    colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]               = withAlpha(bgRaised, 0.98f);
    colors[ImGuiCol_Border]                = border;
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_MenuBarBg]             = bgRaised;

    // Input fields - recessed (darker than the panel) so they read as "type here"
    colors[ImGuiCol_FrameBg]               = bgRecessed;
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.075f, 0.078f, 0.086f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.098f, 0.102f, 0.110f, 1.00f);

    // Title bars - same as the tab strip, so docked areas look like one surface
    colors[ImGuiCol_TitleBg]               = bgBase;
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.075f, 0.078f, 0.086f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = bgBase;

    // Scrollbars
    colors[ImGuiCol_ScrollbarBg]           = bgBase;
    colors[ImGuiCol_ScrollbarGrab]         = controlHover;
    colors[ImGuiCol_ScrollbarGrabHovered]  = controlLight;
    colors[ImGuiCol_ScrollbarGrabActive]   = accent;

    // Checkbox / slider - accent, so values you can change stand out
    colors[ImGuiCol_CheckMark]             = accentLight;
    colors[ImGuiCol_SliderGrab]            = accent;
    colors[ImGuiCol_SliderGrabActive]      = accentLight;

    // Buttons - raised (lighter than the panel), flash accent while pressed
    colors[ImGuiCol_Button]                = control;
    colors[ImGuiCol_ButtonHovered]         = controlHover;
    colors[ImGuiCol_ButtonActive]          = accent;

    // Headers - drives CollapsingHeader in every inspector drawer.
    // Kept neutral so the inspector isn't a wall of blue. The hierarchy and
    // asset window push their own Header colours, so they aren't affected.
    colors[ImGuiCol_Header]                = ImVec4(0.129f, 0.133f, 0.145f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.180f, 0.184f, 0.200f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = withAlpha(accent, 0.60f);

    // Separators / resize grips - quiet at rest, accent while dragging
    colors[ImGuiCol_Separator]             = border;
    colors[ImGuiCol_SeparatorHovered]      = withAlpha(accent, 0.78f);
    colors[ImGuiCol_SeparatorActive]       = accent;
    colors[ImGuiCol_ResizeGrip]            = withAlpha(border, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]     = withAlpha(accent, 0.67f);
    colors[ImGuiCol_ResizeGripActive]      = withAlpha(accent, 0.95f);

    // Tabs - the active tab matches WindowBg so it visually "joins" its panel,
    // and a thin accent line on top marks which one is selected.
    colors[ImGuiCol_Tab]                   = bgBase;
    colors[ImGuiCol_TabHovered]            = control;
    colors[ImGuiCol_TabActive]             = bgWindow;
    colors[ImGuiCol_TabUnfocused]          = bgBase;
    colors[ImGuiCol_TabUnfocusedActive]    = bgWindow;
    colors[ImGuiCol_TabSelectedOverline]   = accent;

    // Docking
    colors[ImGuiCol_DockingPreview]        = withAlpha(accent, 0.70f);
    colors[ImGuiCol_DockingEmptyBg]        = bgBase;

    // Plots - were pure red placeholders
    colors[ImGuiCol_PlotLines]             = ImVec4(0.600f, 0.610f, 0.640f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = accentLight;
    colors[ImGuiCol_PlotHistogram]         = accent;
    colors[ImGuiCol_PlotHistogramHovered]  = accentLight;

    // Tables
    colors[ImGuiCol_TableHeaderBg]         = bgRaised;
    colors[ImGuiCol_TableBorderStrong]     = border;
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.125f, 0.129f, 0.141f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);

    // Drag & drop / keyboard nav / modals - nav and modal dim were pure red
    colors[ImGuiCol_DragDropTarget]        = withAlpha(accentLight, 0.90f);
    colors[ImGuiCol_NavHighlight]          = accent;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // ImGui default
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f); // ImGui default
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.55f);

    applyCommonStyle();
}

// ═══════════════════════════════════════════════════════════════════════════════
// DARK - a softer dark theme (VS-Code-ish mid-grey backgrounds)
// ═══════════════════════════════════════════════════════════════════════════════

void MEditorThemes::applyDarkTheme()
{
    ImVec4* colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.22f, 0.22f, 0.24f, 0.95f);
    colors[ImGuiCol_Border]                = ImVec4(0.30f, 0.30f, 0.32f, 0.50f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.12f, 0.12f, 0.14f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.14f, 0.14f, 0.16f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.36f, 0.36f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.46f, 0.46f, 0.48f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.40f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.40f, 0.70f, 0.90f, 0.80f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.50f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.35f, 0.35f, 0.37f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.32f, 0.32f, 0.34f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.30f, 0.30f, 0.32f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.40f, 0.55f, 0.70f, 0.78f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.40f, 0.55f, 0.70f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.30f, 0.30f, 0.32f, 0.40f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.40f, 0.55f, 0.70f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.40f, 0.55f, 0.70f, 0.90f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_DockingPreview]        = ImVec4(0.40f, 0.70f, 0.90f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.40f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.40f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.50f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.40f, 0.70f, 0.90f, 0.35f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(0.40f, 0.70f, 0.90f, 0.90f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.40f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.55f);

    applyCommonStyle();
}