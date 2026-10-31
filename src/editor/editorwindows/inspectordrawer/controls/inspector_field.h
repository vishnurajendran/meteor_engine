//
// inspector_field.h
//
// One shared layout for every property row in the inspector:
//
//     | Label ............ | value widget(s) ............................ |
//
// The value column starts at the same x for the whole inspector window
// (including nested child boxes such as Behaviour Tuning), so every field
// lines up. Labels that don't fit are cut with "..." and show the full name
// on hover.
//
// Usage:
//
//     START_FIELD_DRAW("Mass (kg)")
//         float mass = body->mass.get();
//         if (ImGui::DragFloat("##v", &mass))
//             body->mass.set(mass);
//     END_FIELD_DRAW()
//
// Optional extras after the label: a tooltip, a label colour, or both.
//
//     START_FIELD_DRAW("Half Height", "Cylindrical segment only.")
//     START_FIELD_DRAW("Speed", overriddenColor)
//     START_FIELD_DRAW("Speed", overriddenColor, "Overrides the script default")
//
// The block between the macros is its own scope, so locals don't clash
// between fields. Widgets inside fill the value column by default (the item
// width is -FLT_MIN); set a width yourself to leave room for extra widgets.
// Each field pushes an ID from its label, so "##v" style IDs are safe to
// reuse from field to field. Use MInspectorField::DragFloat / DragInt /
// SliderFloat instead of the ImGui ones to get left-aligned values.
//
#pragma once
#ifndef METEOR_ENGINE_INSPECTOR_FIELD_H
#define METEOR_ENGINE_INSPECTOR_FIELD_H

#include <string>
#include "imgui.h"

namespace sf { class Texture; }

enum class EInspectorStatus
{
    Info,
    Warning,
    Error,
};

namespace MInspectorField
{
    // Fraction of the inspector width given to labels, and its limits in px.
    inline constexpr float LABEL_FRACTION  = 0.40f;
    inline constexpr float LABEL_MIN_WIDTH = 90.0f;
    inline constexpr float LABEL_MAX_WIDTH = 220.0f;

    void beginField(const char* label, const char* tooltip = nullptr);
    void beginField(const char* label, const ImVec4& labelColor, const char* tooltip = nullptr);
    inline void beginField(const std::string& label, const char* tooltip = nullptr) { beginField(label.c_str(), tooltip); }
    inline void beginField(const std::string& label, const ImVec4& labelColor, const char* tooltip = nullptr) { beginField(label.c_str(), labelColor, tooltip); }

    void endField();

    // Screen-space x where the value column starts in the current window.
    float getValueColumnX();

    // Gives the next field a third, fixed-width cell on the right (e.g. for a
    // reset button). Every row of a group should reserve it, used or not, so
    // the columns stay even. Inside the field, call trailingCell() to move
    // the cursor into it.
    void setNextFieldTrailingCell(float width);
    void trailingCell();

    // A full-width row with a single caption, for sub-sections inside the grid.
    void headerRow(const char* text);

    // Same as the ImGui widgets, but the value text is left-aligned instead
    // of centred (ImGui hard-codes centring for drags and sliders).
    bool DragFloat(const char* label, float* v, float speed = 1.0f, float min = 0.0f, float max = 0.0f,
                   const char* format = "%.3f", ImGuiSliderFlags flags = 0);
    bool DragInt(const char* label, int* v, float speed = 1.0f, int min = 0, int max = 0,
                 const char* format = "%d", ImGuiSliderFlags flags = 0);
    bool SliderFloat(const char* label, float* v, float min, float max,
                     const char* format = "%.3f", ImGuiSliderFlags flags = 0);

    // Square, frame-height "reset to default" button using the undo icon
    // (falls back to a "Reset" text button if the icon can't be loaded).
    bool ResetButton(const char* id, const char* tooltip = "Reset to default");
    float getResetButtonWidth();

    // Square, frame-height remove/clear button using the remove icon (falls
    // back to an "x" text button if the icon can't be loaded).
    bool RemoveButton(const char* id, const char* tooltip = "Remove");

    // 16x16 status icon, vertically centred on a frame-height line. Shows
    // `tooltip` on hover. Put ImGui::SameLine() after it to continue the line.
    inline constexpr float STATUS_ICON_SIZE = 16.0f;
    void statusIcon(EInspectorStatus status, const char* tooltip = nullptr);

    // Icon followed by text in the matching colour, on one line.
    void statusText(EInspectorStatus status, const char* text, const char* tooltip = nullptr);

    // The raw icon texture (nullptr if it failed to load), for controls that
    // position things themselves.
    const sf::Texture* getStatusIconTexture(EInspectorStatus status);
    ImVec4 getStatusColor(EInspectorStatus status);
}

#define START_FIELD_DRAW(...) ::MInspectorField::beginField(__VA_ARGS__); {
#define END_FIELD_DRAW()      } ::MInspectorField::endField();

#endif // METEOR_ENGINE_INSPECTOR_FIELD_H
