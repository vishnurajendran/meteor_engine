//
// inspector_field.cpp
//

#include "inspector_field.h"
#include <algorithm>
#include <vector>
#include "imgui_internal.h"
#include "imgui-SFML.h"
#include "SFML/Graphics/Texture.hpp"
#include "default_engine_icon_paths.h"
#include "editor/editor_utils/engine_textures.h"

namespace
{
    // Horizontal padding inside a cell, from a border to its content.
    constexpr float CELL_PAD_X = 6.0f;
    // Extra vertical padding inside a row, above and below its content.
    constexpr float CELL_PAD_Y = 2.0f;
    // How far the outer border sits outside the window's work rect. Must stay
    // inside the window's clip rect (half the window padding).
    constexpr float BORDER_OUTSET = 3.0f;
    // Grid lines: the window background nudged toward the text colour, so the
    // line is a lighter shade of the panel in any theme.
    constexpr float LINE_TINT = 0.015f;

    struct SFieldRow
    {
        float top;          // cursor y when the row began
        float contentY;     // y of the row's first line of content
        float left;         // left border x
        float right;        // right border x
        float columnX;      // label | value divider x (0 = no divider)
        float trailingX;    // value | trailing divider x (0 = no trailing cell)
        float workMaxX;     // window work rect max x to restore
        float contentMaxX;  // window content region max x to restore
    };
    std::vector<SFieldRow> gRowStack;
    float                  gNextTrailingWidth = 0.0f;

    ImU32 lineColor()
    {
        // Built from style colours directly (not GetColorU32) so lines don't
        // fade inside BeginDisabled().
        const ImVec4& bg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        const ImVec4& fg = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        return ImGui::ColorConvertFloat4ToU32(ImVec4(ImLerp(bg.x, fg.x, LINE_TINT),
                                                     ImLerp(bg.y, fg.y, LINE_TINT),
                                                     ImLerp(bg.z, fg.z, LINE_TINT), 1.0f));
    }

    // Limits how far widgets in the current cell extend: GetContentRegionAvail,
    // -FLT_MIN item widths and self-sizing controls all stop at `maxX`.
    void setCellRightEdge(float maxX)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        window->WorkRect.Max.x          = maxX;
        window->ContentRegionRect.Max.x = maxX;
    }

    // Starts a row: records geometry and steps past the top padding.
    SFieldRow& pushRow(float columnX, float trailingWidth)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImVec2 start  = ImGui::GetCursorScreenPos();

        SFieldRow row{};
        row.top         = start.y;
        row.contentY    = start.y + CELL_PAD_Y;
        row.left        = start.x - BORDER_OUTSET;
        row.right       = window->WorkRect.Max.x + BORDER_OUTSET;
        row.columnX     = columnX;
        row.trailingX   = trailingWidth > 0.0f ? row.right - trailingWidth - CELL_PAD_X * 2.0f : 0.0f;
        row.workMaxX    = window->WorkRect.Max.x;
        row.contentMaxX = window->ContentRegionRect.Max.x;
        gRowStack.push_back(row);

        ImGui::SetCursorScreenPos(ImVec2(start.x, row.contentY));
        return gRowStack.back();
    }

    // Ends a row: draws its borders and steps past the bottom padding.
    void popRow()
    {
        ImGuiWindow*      window = ImGui::GetCurrentWindow();
        const ImGuiStyle& style  = ImGui::GetStyle();
        const SFieldRow   row    = gRowStack.back();
        gRowStack.pop_back();

        window->WorkRect.Max.x          = row.workMaxX;
        window->ContentRegionRect.Max.x = row.contentMaxX;

        // Borders sit in the middle of the item spacing, so neighbouring rows
        // share one line.
        const float bottom  = ImGui::GetItemRectMax().y + CELL_PAD_Y;
        const float halfGap = style.ItemSpacing.y * 0.5f;
        const float y0      = row.top - halfGap;
        const float y1      = bottom + halfGap;

        ImDrawList* dl   = window->DrawList;
        const ImU32 line = lineColor();
        dl->AddRect(ImVec2(row.left, y0), ImVec2(row.right, y1), line);
        if (row.columnX > 0.0f)
            dl->AddLine(ImVec2(row.columnX, y0), ImVec2(row.columnX, y1), line);
        if (row.trailingX > 0.0f)
            dl->AddLine(ImVec2(row.trailingX, y0), ImVec2(row.trailingX, y1), line);

        // Step the cursor past the bottom padding.
        window->DC.CursorPos.y    += CELL_PAD_Y;
        window->DC.CursorMaxPos.y  = std::max(window->DC.CursorMaxPos.y, window->DC.CursorPos.y);
    }

    struct SLazyIcon
    {
        sf::Texture texture;
        bool        tried  = false;
        bool        loaded = false;

        // Load once; a missing icon shouldn't retry (and warn) every frame.
        const sf::Texture* get(const char* path)
        {
            if (!tried)
            {
                tried  = true;
                loaded = loadEngineTexture(texture, path);
            }
            return loaded ? &texture : nullptr;
        }
    };

    const sf::Texture* undoIcon()
    {
        static SLazyIcon icon;
        return icon.get(SEngineAssetIconPaths::LOWRES_TEX_BTTN_UNDO);
    }

    const sf::Texture* removeIcon()
    {
        static SLazyIcon icon;
        return icon.get(SEngineAssetIconPaths::LOWRES_TEX_BTTN_REMOVE);
    }

    // Square button exactly one frame height tall: the icon fills it minus the
    // vertical frame padding on every side.
    bool squareIconButton(const char* id, const sf::Texture& icon)
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float       side  = ImGui::GetFrameHeight() - style.FramePadding.y * 2.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.y, style.FramePadding.y));
        const bool clicked = ImGui::ImageButton(id, icon, sf::Vector2f(side, side));
        ImGui::PopStyleVar();
        return clicked;
    }

    // Draws an ImGui drag/slider with its text hidden, then draws the value
    // left-aligned in the frame. While the widget is in text-input mode
    // (double-click / ctrl+click) it is drawn untouched.
    template <typename T, typename DrawFn>
    bool drawLeftAligned(const char* label, const T* v, const char* format, DrawFn&& draw)
    {
        if (ImGui::TempInputIsActive(ImGui::GetID(label)))
            return draw();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        const bool changed = draw();
        ImGui::PopStyleColor();

        char buf[64];
        ImFormatString(buf, IM_ARRAYSIZE(buf), format, *v);

        const ImGuiStyle& style = ImGui::GetStyle();
        const ImVec2      min   = ImGui::GetItemRectMin();
        const ImVec2      max   = ImGui::GetItemRectMax();
        ImGui::RenderTextClipped(ImVec2(min.x + style.FramePadding.x, min.y),
                                 ImVec2(max.x - style.FramePadding.x, max.y),
                                 buf, nullptr, nullptr, ImVec2(0.0f, 0.5f));
        return changed;
    }

    void beginFieldImpl(const char* label, const ImVec4* labelColor, const char* tooltip)
    {
        ImGuiWindow*      window = ImGui::GetCurrentWindow();
        const ImGuiStyle& style  = ImGui::GetStyle();

        ImGui::PushID(label);

        const float startX  = ImGui::GetCursorScreenPos().x;
        const float columnX = std::max(MInspectorField::getValueColumnX(), startX + 40.0f);
        const SFieldRow& row = pushRow(columnX, gNextTrailingWidth);
        gNextTrailingWidth = 0.0f;

        // Label cell.
        const float frameH  = ImGui::GetFrameHeight();
        const float textMin = row.left + CELL_PAD_X;
        const float textMax = columnX - CELL_PAD_X;

        ImGui::Dummy(ImVec2(columnX - startX, frameH));

        const char*  labelEnd = ImGui::FindRenderedTextEnd(label);
        const ImVec2 textSize = ImGui::CalcTextSize(label, labelEnd);
        const bool   clipped  = textMin + textSize.x > textMax;

        if (labelColor)
            ImGui::PushStyleColor(ImGuiCol_Text, *labelColor);
        ImGui::RenderTextEllipsis(window->DrawList,
                                  ImVec2(textMin, row.contentY + style.FramePadding.y),
                                  ImVec2(textMax, row.contentY + frameH),
                                  textMax, textMax,
                                  label, labelEnd, &textSize);
        if (labelColor)
            ImGui::PopStyleColor();

        if ((clipped || tooltip) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            if (clipped)
                ImGui::TextUnformatted(label, labelEnd);
            if (clipped && tooltip)
                ImGui::Separator();
            if (tooltip)
                ImGui::TextUnformatted(tooltip);
            ImGui::EndTooltip();
        }

        // Value cell: widgets fill it and stop CELL_PAD_X short of the next
        // border, whatever they are.
        const float cellRight = row.trailingX > 0.0f ? row.trailingX : row.right;
        setCellRightEdge(cellRight - CELL_PAD_X);

        ImGui::SameLine(0.0f, CELL_PAD_X);
        ImGui::AlignTextToFramePadding();
        ImGui::BeginGroup();
        ImGui::PushItemWidth(-FLT_MIN);
    }
}

namespace MInspectorField
{
    float getValueColumnX()
    {
        // Measured against the top-level window, so every row lines up.
        const ImGuiWindow* root  = ImGui::GetCurrentWindow()->RootWindow;
        const float        width = root->WorkRect.GetWidth();
        return root->WorkRect.Min.x + std::clamp(width * LABEL_FRACTION, LABEL_MIN_WIDTH, LABEL_MAX_WIDTH);
    }

    void beginField(const char* label, const char* tooltip)
    {
        beginFieldImpl(label, nullptr, tooltip);
    }

    void beginField(const char* label, const ImVec4& labelColor, const char* tooltip)
    {
        beginFieldImpl(label, &labelColor, tooltip);
    }

    void endField()
    {
        ImGui::PopItemWidth();
        ImGui::EndGroup();
        popRow();
        ImGui::PopID();
    }

    void setNextFieldTrailingCell(float width)
    {
        gNextTrailingWidth = width;
    }

    void trailingCell()
    {
        IM_ASSERT(!gRowStack.empty() && gRowStack.back().trailingX > 0.0f &&
                  "trailingCell() needs setNextFieldTrailingCell() before START_FIELD_DRAW");
        const SFieldRow& row = gRowStack.back();

        setCellRightEdge(row.right - CELL_PAD_X);
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetCursorScreenPos(ImVec2(row.trailingX + CELL_PAD_X, row.contentY));
    }

    void headerRow(const char* text)
    {
        ImGui::PushID(text);
        const SFieldRow& row = pushRow(0.0f, 0.0f);

        const float frameH = ImGui::GetFrameHeight();
        const float width  = row.right - row.left - BORDER_OUTSET * 2.0f;
        ImGui::Dummy(ImVec2(width, frameH));

        const ImVec2 textPos(row.left + CELL_PAD_X, row.contentY + ImGui::GetStyle().FramePadding.y);
        ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), text);

        popRow();
        ImGui::PopID();
    }

    bool DragFloat(const char* label, float* v, float speed, float min, float max,
                   const char* format, ImGuiSliderFlags flags)
    {
        return drawLeftAligned(label, v, format, [&] { return ImGui::DragFloat(label, v, speed, min, max, format, flags); });
    }

    bool DragInt(const char* label, int* v, float speed, int min, int max,
                 const char* format, ImGuiSliderFlags flags)
    {
        return drawLeftAligned(label, v, format, [&] { return ImGui::DragInt(label, v, speed, min, max, format, flags); });
    }

    bool SliderFloat(const char* label, float* v, float min, float max,
                     const char* format, ImGuiSliderFlags flags)
    {
        return drawLeftAligned(label, v, format, [&] { return ImGui::SliderFloat(label, v, min, max, format, flags); });
    }

    float getResetButtonWidth()
    {
        if (undoIcon())
            return ImGui::GetFrameHeight();
        return ImGui::CalcTextSize("Reset").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    }

    bool ResetButton(const char* id, const char* tooltip)
    {
        bool clicked;
        if (const sf::Texture* icon = undoIcon())
        {
            clicked = squareIconButton(id, *icon);
        }
        else
        {
            ImGui::PushID(id);
            clicked = ImGui::Button("Reset", ImVec2(getResetButtonWidth(), 0.0f));
            ImGui::PopID();
        }

        if (tooltip && ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", tooltip);
        return clicked;
    }

    const sf::Texture* getStatusIconTexture(EInspectorStatus status)
    {
        static SLazyIcon info, warning, error;
        switch (status)
        {
            case EInspectorStatus::Info:    return info.get(SEngineAssetIconPaths::LOWRES_TEX_STATUS_INFO);
            case EInspectorStatus::Warning: return warning.get(SEngineAssetIconPaths::LOWRES_TEX_STATUS_WARNING);
            case EInspectorStatus::Error:   return error.get(SEngineAssetIconPaths::LOWRES_TEX_STATUS_ERROR);
        }
        return nullptr;
    }

    ImVec4 getStatusColor(EInspectorStatus status)
    {
        switch (status)
        {
            case EInspectorStatus::Warning: return ImVec4(0.95f, 0.75f, 0.30f, 1.0f);
            case EInspectorStatus::Error:   return ImVec4(0.95f, 0.40f, 0.40f, 1.0f);
            case EInspectorStatus::Info:
            default:                        return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        }
    }

    void statusIcon(EInspectorStatus status, const char* tooltip)
    {
        const ImVec2 pos   = ImGui::GetCursorScreenPos();
        const float  lineH = ImGui::GetFrameHeight();

        // Draw the icon centred on the line, then reserve the full-height cell
        // last so SameLine() after this lines up with frame-height widgets.
        if (const sf::Texture* icon = getStatusIconTexture(status))
        {
            ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + (lineH - STATUS_ICON_SIZE) * 0.5f));
            ImGui::Image(*icon, sf::Vector2f(STATUS_ICON_SIZE, STATUS_ICON_SIZE));
            ImGui::SetCursorScreenPos(pos);
        }
        ImGui::Dummy(ImVec2(STATUS_ICON_SIZE, lineH));
        const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);

        if (tooltip && hovered)
            ImGui::SetTooltip("%s", tooltip);
    }

    void statusText(EInspectorStatus status, const char* text, const char* tooltip)
    {
        ImGui::BeginGroup();
        statusIcon(status);
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(getStatusColor(status), "%s", text);
        ImGui::EndGroup();

        if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s", tooltip);
    }

    bool RemoveButton(const char* id, const char* tooltip)
    {
        bool clicked;
        if (const sf::Texture* icon = removeIcon())
        {
            clicked = squareIconButton(id, *icon);
        }
        else
        {
            const float side = ImGui::GetFrameHeight();
            ImGui::PushID(id);
            clicked = ImGui::Button("x", ImVec2(side, side));
            ImGui::PopID();
        }

        if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s", tooltip);
        return clicked;
    }
}
