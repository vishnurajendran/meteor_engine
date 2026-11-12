//
// curve_gradient_controls.cpp
//

#include "curve_gradient_controls.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "imgui.h"

namespace
{
    constexpr float KEY_RADIUS        = 4.5f;
    constexpr float KEY_PICK_RADIUS   = 7.0f;    // px — how close the mouse must be to grab a key
    constexpr float CURVE_CANVAS_W    = 320.0f;
    constexpr float CURVE_CANVAS_H    = 180.0f;
    constexpr float GRADIENT_BAR_H    = 24.0f;
    constexpr float MARKER_ROW_H      = 14.0f;
    constexpr float CHECKER_CELL      = 6.0f;
    constexpr float MIN_RANGE         = 1e-4f;   // keeps the graph's value range from collapsing to zero

    ImU32 toU32(const SVector4& c)
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(std::clamp(c.r, 0.0f, 1.0f), std::clamp(c.g, 0.0f, 1.0f),
                                                     std::clamp(c.b, 0.0f, 1.0f), std::clamp(c.a, 0.0f, 1.0f)));
    }

    float distance(const ImVec2& a, const ImVec2& b)
    {
        const float dx = a.x - b.x, dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    // ---- Shared drawing ---------------------------------------------------------

    // A piecewise-linear curve is exactly its keys joined by lines, held flat
    // to t = 0 and t = 1 — so draw those points instead of sampling.
    void drawCurveLine(ImDrawList* dl, const SCurve& curve, const ImVec2& p0, const ImVec2& p1,
                       float lo, float hi, ImU32 col, float thickness)
    {
        if (curve.keys.empty()) return;

        const float w     = p1.x - p0.x;
        const float h     = p1.y - p0.y;
        const float range = std::max(hi - lo, MIN_RANGE);
        auto toScreen = [&](float t, float v) { return ImVec2(p0.x + t * w, p1.y - (v - lo) / range * h); };

        std::vector<ImVec2> pts;
        pts.reserve(curve.keys.size() + 2);
        pts.push_back(toScreen(0.0f, curve.evaluate(0.0f)));
        for (const SCurveKey& k : curve.keys) pts.push_back(toScreen(k.time, k.value));
        pts.push_back(toScreen(1.0f, curve.evaluate(1.0f)));

        dl->AddPolyline(pts.data(), static_cast<int>(pts.size()), col, ImDrawFlags_None, thickness);
    }

    // Checkerboard behind gradients so alpha is visible.
    void drawChecker(ImDrawList* dl, const ImVec2& p0, const ImVec2& p1)
    {
        dl->AddRectFilled(p0, p1, IM_COL32(200, 200, 200, 255));
        int row = 0;
        for (float y = p0.y; y < p1.y; y += CHECKER_CELL, ++row)
        {
            int col = row;
            for (float x = p0.x; x < p1.x; x += CHECKER_CELL, ++col)
            {
                if (col % 2 == 0) continue;
                dl->AddRectFilled(ImVec2(x, y),
                                  ImVec2(std::min(x + CHECKER_CELL, p1.x), std::min(y + CHECKER_CELL, p1.y)),
                                  IM_COL32(140, 140, 140, 255));
            }
        }
    }

    // Like the curve, a linear gradient is exact as one coloured quad per
    // pair of neighbouring stops.
    void drawGradientBar(ImDrawList* dl, const SGradient& g, const ImVec2& p0, const ImVec2& p1)
    {
        drawChecker(dl, p0, p1);

        const float w = p1.x - p0.x;
        std::vector<std::pair<float, SVector4>> stops;
        stops.reserve(g.keys.size() + 2);
        stops.emplace_back(0.0f, g.evaluate(0.0f));
        for (const SGradientKey& k : g.keys) stops.emplace_back(k.time, k.color);
        stops.emplace_back(1.0f, g.evaluate(1.0f));

        for (size_t i = 0; i + 1 < stops.size(); ++i)
        {
            const float x0 = p0.x + stops[i].first * w;
            const float x1 = p0.x + stops[i + 1].first * w;
            if (x1 - x0 < 0.5f) continue;   // zero-width step
            const ImU32 a = toU32(stops[i].second);
            const ImU32 b = toU32(stops[i + 1].second);
            dl->AddRectFilledMultiColor(ImVec2(x0, p0.y), ImVec2(x1, p1.y), a, b, b, a);
        }
        dl->AddRect(p0, p1, ImGui::GetColorU32(ImGuiCol_Border));
    }

    // The value column preview both fields use. Returns true when clicked.
    bool previewButton(const char* id, float& outWidth, ImVec2& outP0, ImVec2& outP1)
    {
        outWidth = std::max(ImGui::CalcItemWidth(), 40.0f);
        const float height = ImGui::GetFrameHeight();
        outP0 = ImGui::GetCursorScreenPos();
        outP1 = ImVec2(outP0.x + outWidth, outP0.y + height);
        const bool clicked = ImGui::InvisibleButton(id, ImVec2(outWidth, height));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Click to edit");
        return clicked;
    }

    // ---- Curve editor popup -------------------------------------------------------

    bool drawCurveEditor(SCurve& curve, float viewMin, float viewMax)
    {
        bool changed = false;

        ImGuiStorage* st     = ImGui::GetStateStorage();
        const ImGuiID selId  = ImGui::GetID("##sel");
        const ImGuiID dragId = ImGui::GetID("##drag");
        const ImGuiID loId   = ImGui::GetID("##lo");
        const ImGuiID hiId   = ImGui::GetID("##hi");

        // On open: frame the keys, but never smaller than the caller's range.
        if (ImGui::IsWindowAppearing())
        {
            float kMin, kMax;
            curve.getValueRange(kMin, kMax);
            st->SetFloat(loId, std::min(viewMin, kMin));
            st->SetFloat(hiId, std::max(viewMax, kMax));
            st->SetInt(selId, -1);
            st->SetBool(dragId, false);
        }

        int   sel = st->GetInt(selId, -1);
        float lo  = st->GetFloat(loId, viewMin);
        float hi  = st->GetFloat(hiId, viewMax);
        if (hi - lo < MIN_RANGE) hi = lo + MIN_RANGE;
        if (sel >= static_cast<int>(curve.keys.size())) sel = -1;

        // ---- Canvas ---------------------------------------------------------------
        const ImVec2 size(CURVE_CANVAS_W, CURVE_CANVAS_H);
        const ImVec2 p0 = ImGui::GetCursorScreenPos();
        const ImVec2 p1(p0.x + size.x, p0.y + size.y);
        ImGui::InvisibleButton("##canvas", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        const bool hovered = ImGui::IsItemHovered();
        const ImVec2 mouse = ImGui::GetIO().MousePos;

        auto toScreen = [&](float t, float v) { return ImVec2(p0.x + t * size.x, p1.y - (v - lo) / (hi - lo) * size.y); };
        auto fromScreen = [&](const ImVec2& s, float& t, float& v)
        {
            t = (s.x - p0.x) / size.x;
            v = lo + (p1.y - s.y) / size.y * (hi - lo);
        };

        // Key under the mouse — the closest one within the pick radius.
        int hoverKey = -1;
        if (hovered)
        {
            float best = KEY_PICK_RADIUS;
            for (int i = 0; i < static_cast<int>(curve.keys.size()); ++i)
            {
                const float d = distance(mouse, toScreen(curve.keys[i].time, curve.keys[i].value));
                if (d < best) { best = d; hoverKey = i; }
            }
        }

        // Press: select (and start dragging) the key under the mouse.
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            sel = hoverKey;
            st->SetBool(dragId, hoverKey >= 0);
        }

        // Double-click on empty space: add a key there.
        if (hovered && hoverKey < 0 && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            float t, v;
            fromScreen(mouse, t, v);
            sel = curve.addKey(t, std::clamp(v, lo, hi));
            st->SetBool(dragId, false);
            changed = true;
        }

        // Right-click a key: remove it (always keep one).
        if (hovered && hoverKey >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && curve.keys.size() > 1)
        {
            curve.removeKey(hoverKey);
            sel = -1;
            changed = true;
        }

        // Neighbours bound the selected key's time, so dragging never
        // reorders keys and `sel` stays valid.
        auto timeBounds = [&](int i, float& tMin, float& tMax)
        {
            tMin = i > 0 ? curve.keys[i - 1].time : 0.0f;
            tMax = i + 1 < static_cast<int>(curve.keys.size()) ? curve.keys[i + 1].time : 1.0f;
        };

        if (st->GetBool(dragId) && sel >= 0)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                float t, v, tMin, tMax;
                fromScreen(mouse, t, v);
                timeBounds(sel, tMin, tMax);
                t = std::clamp(t, tMin, tMax);
                v = std::clamp(v, lo, hi);
                SCurveKey& k = curve.keys[sel];
                if (t != k.time || v != k.value) { k.time = t; k.value = v; changed = true; }
            }
            else
            {
                st->SetBool(dragId, false);
            }
        }

        // ---- Draw -------------------------------------------------------------------
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_FrameBg));
        const ImU32 gridCol = ImGui::GetColorU32(ImGuiCol_Border);
        for (int i = 1; i < 4; ++i)
        {
            const float x = p0.x + size.x * i / 4.0f;
            const float y = p0.y + size.y * i / 4.0f;
            dl->AddLine(ImVec2(x, p0.y), ImVec2(x, p1.y), gridCol);
            dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y), gridCol);
        }
        if (lo < 0.0f && hi > 0.0f)   // zero line
        {
            const float y = toScreen(0.0f, 0.0f).y;
            dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y), ImGui::GetColorU32(ImGuiCol_TextDisabled), 1.5f);
        }

        dl->PushClipRect(p0, p1, true);
        drawCurveLine(dl, curve, p0, p1, lo, hi, ImGui::GetColorU32(ImGuiCol_PlotLines), 2.0f);
        for (int i = 0; i < static_cast<int>(curve.keys.size()); ++i)
        {
            const ImVec2 c   = toScreen(curve.keys[i].time, curve.keys[i].value);
            const ImU32  col = i == sel      ? ImGui::GetColorU32(ImGuiCol_PlotLinesHovered)
                             : i == hoverKey ? ImGui::GetColorU32(ImGuiCol_Text)
                                             : ImGui::GetColorU32(ImGuiCol_PlotLines);
            dl->AddCircleFilled(c, KEY_RADIUS, col);
        }
        dl->PopClipRect();
        dl->AddRect(p0, p1, gridCol);

        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", hi);
        dl->AddText(ImVec2(p0.x + 3, p0.y + 2), ImGui::GetColorU32(ImGuiCol_TextDisabled), buf);
        snprintf(buf, sizeof(buf), "%.2f", lo);
        dl->AddText(ImVec2(p0.x + 3, p1.y - ImGui::GetTextLineHeight() - 2), ImGui::GetColorU32(ImGuiCol_TextDisabled), buf);

        // ---- Selected key -----------------------------------------------------------
        if (sel >= 0)
        {
            SCurveKey& k = curve.keys[sel];
            float tMin, tMax;
            timeBounds(sel, tMin, tMax);

            ImGui::SetNextItemWidth(110.0f);
            if (ImGui::DragFloat("Time", &k.time, 0.005f, tMin, tMax, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                changed = true;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(110.0f);
            if (ImGui::DragFloat("Value", &k.value, (hi - lo) * 0.005f, 0.0f, 0.0f, "%.3f"))
                changed = true;
        }
        else
        {
            ImGui::TextDisabled("Double-click: add key   Right-click key: remove");
        }

        // ---- Range + presets --------------------------------------------------------
        ImGui::SetNextItemWidth(110.0f);
        ImGui::DragFloat("Min", &lo, 0.01f, 0.0f, 0.0f, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110.0f);
        ImGui::DragFloat("Max", &hi, 0.01f, 0.0f, 0.0f, "%.2f");
        if (hi - lo < MIN_RANGE) hi = lo + MIN_RANGE;

        if (ImGui::SmallButton("Flat"))     { curve = SCurve::constant(hi);    sel = -1; changed = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Rise"))     { curve = SCurve::linear(lo, hi);  sel = -1; changed = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Fall"))     { curve = SCurve::linear(hi, lo);  sel = -1; changed = true; }

        st->SetInt(selId, sel);
        st->SetFloat(loId, lo);
        st->SetFloat(hiId, hi);
        return changed;
    }

    // ---- Gradient editor popup ----------------------------------------------------

    bool drawGradientEditor(SGradient& g)
    {
        bool changed = false;

        ImGuiStorage* st     = ImGui::GetStateStorage();
        const ImGuiID selId  = ImGui::GetID("##sel");
        const ImGuiID dragId = ImGui::GetID("##drag");
        if (ImGui::IsWindowAppearing())
        {
            st->SetInt(selId, g.keys.empty() ? -1 : 0);
            st->SetBool(dragId, false);
        }

        int sel = st->GetInt(selId, -1);
        if (sel >= static_cast<int>(g.keys.size())) sel = -1;

        const float  w  = CURVE_CANVAS_W;
        const ImVec2 p0 = ImGui::GetCursorScreenPos();
        const ImVec2 barP1(p0.x + w, p0.y + GRADIENT_BAR_H);
        ImGui::InvisibleButton("##bar", ImVec2(w, GRADIENT_BAR_H + MARKER_ROW_H),
                               ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        const bool   hovered = ImGui::IsItemHovered();
        const ImVec2 mouse   = ImGui::GetIO().MousePos;

        auto timeAt = [&](float x) { return std::clamp((x - p0.x) / w, 0.0f, 1.0f); };

        // Marker under the mouse — anywhere in the column above / below it.
        int hoverKey = -1;
        if (hovered)
        {
            float best = KEY_PICK_RADIUS;
            for (int i = 0; i < static_cast<int>(g.keys.size()); ++i)
            {
                const float d = std::fabs(mouse.x - (p0.x + g.keys[i].time * w));
                if (d < best) { best = d; hoverKey = i; }
            }
        }

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            if (hoverKey >= 0) sel = hoverKey;
            st->SetBool(dragId, hoverKey >= 0);
        }

        if (hovered && hoverKey < 0 && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            const float t = timeAt(mouse.x);
            sel = g.addKey(t, g.evaluate(t));   // new key keeps the current colour there
            st->SetBool(dragId, false);
            changed = true;
        }

        if (hovered && hoverKey >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && g.keys.size() > 1)
        {
            g.removeKey(hoverKey);
            sel = -1;
            changed = true;
        }

        auto timeBounds = [&](int i, float& tMin, float& tMax)
        {
            tMin = i > 0 ? g.keys[i - 1].time : 0.0f;
            tMax = i + 1 < static_cast<int>(g.keys.size()) ? g.keys[i + 1].time : 1.0f;
        };

        if (st->GetBool(dragId) && sel >= 0)
        {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                float tMin, tMax;
                timeBounds(sel, tMin, tMax);
                const float t = std::clamp(timeAt(mouse.x), tMin, tMax);
                if (t != g.keys[sel].time) { g.keys[sel].time = t; changed = true; }
            }
            else
            {
                st->SetBool(dragId, false);
            }
        }

        // ---- Draw ---------------------------------------------------------------------
        ImDrawList* dl = ImGui::GetWindowDrawList();
        drawGradientBar(dl, g, p0, barP1);
        for (int i = 0; i < static_cast<int>(g.keys.size()); ++i)
        {
            const float x   = p0.x + g.keys[i].time * w;
            const float top = barP1.y + 1.0f;
            const ImVec2 a(x, top), b(x - 5.0f, top + MARKER_ROW_H - 3.0f), c(x + 5.0f, top + MARKER_ROW_H - 3.0f);
            SVector4 opaque = g.keys[i].color;
            opaque.a = 1.0f;
            dl->AddTriangleFilled(a, b, c, toU32(opaque));
            const ImU32 outline = i == sel      ? ImGui::GetColorU32(ImGuiCol_PlotLinesHovered)
                                : i == hoverKey ? ImGui::GetColorU32(ImGuiCol_Text)
                                                : ImGui::GetColorU32(ImGuiCol_Border);
            dl->AddTriangle(a, b, c, outline, i == sel ? 2.0f : 1.0f);
        }

        // ---- Selected key -------------------------------------------------------------
        if (sel >= 0)
        {
            SGradientKey& k = g.keys[sel];
            ImGui::SetNextItemWidth(w);
            if (ImGui::ColorEdit4("##color", &k.color.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float))
                changed = true;

            float tMin, tMax;
            timeBounds(sel, tMin, tMax);
            ImGui::SetNextItemWidth(110.0f);
            if (ImGui::DragFloat("Time", &k.time, 0.005f, tMin, tMax, "%.3f", ImGuiSliderFlags_AlwaysClamp))
                changed = true;
        }
        else
        {
            ImGui::TextDisabled("Double-click bar: add key   Right-click marker: remove");
        }

        if (ImGui::SmallButton("Fade Out"))
        {
            g = SGradient::linear(SVector4(1.0f), SVector4(1.0f, 1.0f, 1.0f, 0.0f));
            sel = 0;
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Solid White"))
        {
            g = SGradient::constant(SVector4(1.0f));
            sel = 0;
            changed = true;
        }

        st->SetInt(selId, sel);
        return changed;
    }
}

namespace MCurveGradientControls
{
    bool curveField(const char* id, SCurve& curve, float viewMin, float viewMax)
    {
        ImGui::PushID(id);

        float  width;
        ImVec2 p0, p1;
        const bool clicked = previewButton("##preview", width, p0, p1);

        // Preview uses the same framing rule as the popup.
        float kMin, kMax;
        curve.getValueRange(kMin, kMax);
        const float lo = std::min(viewMin, kMin);
        const float hi = std::max(viewMax, kMax);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p0, p1, ImGui::GetColorU32(ImGuiCol_FrameBg), ImGui::GetStyle().FrameRounding);
        dl->PushClipRect(p0, p1, true);
        drawCurveLine(dl, curve, ImVec2(p0.x + 2, p0.y + 2), ImVec2(p1.x - 2, p1.y - 2), lo, hi,
                      ImGui::GetColorU32(ImGuiCol_PlotLines), 1.5f);
        dl->PopClipRect();

        if (clicked) ImGui::OpenPopup("##curveEditor");

        bool changed = false;
        if (ImGui::BeginPopup("##curveEditor"))
        {
            changed = drawCurveEditor(curve, viewMin, viewMax);
            ImGui::EndPopup();
        }

        ImGui::PopID();
        return changed;
    }

    bool gradientField(const char* id, SGradient& gradient)
    {
        ImGui::PushID(id);

        float  width;
        ImVec2 p0, p1;
        const bool clicked = previewButton("##preview", width, p0, p1);
        drawGradientBar(ImGui::GetWindowDrawList(), gradient, p0, p1);

        if (clicked) ImGui::OpenPopup("##gradientEditor");

        bool changed = false;
        if (ImGui::BeginPopup("##gradientEditor"))
        {
            changed = drawGradientEditor(gradient);
            ImGui::EndPopup();
        }

        ImGui::PopID();
        return changed;
    }
}
