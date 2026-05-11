//
// editorthemes.h
//
// Reads MEditorSettings::theme and applies the matching ImGui colour scheme.
// The theme map is a static std::map<string, function> so new themes can be
// added in one place without touching dispatch logic.
//
// Suggested location: src/editor/window/imgui/editorthemes.h
//

#ifndef EDITOR_THEMES_H
#define EDITOR_THEMES_H

#include <functional>
#include <map>
#include <string>

class MEditorThemes
{
public:
    // Reads MEditorSettings::get().theme and calls the matching function.
    // Falls back to midnight if the key is unrecognised.
    static void applyTheme();

private:
    static void applyMidnightTheme();
    static void applyDarkTheme();

    // Shared style geometry (rounding, padding, spacing).
    // Called by every theme so layout stays consistent.
    static void applyCommonStyle();

    static const std::map<std::string, std::function<void()>>& getThemeMap();
};

#endif // EDITOR_THEMES_H