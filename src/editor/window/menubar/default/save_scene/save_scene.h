//
// save_scene.h
//

#ifndef SAVE_SCENE_MENUBAR_ITEM_H
#define SAVE_SCENE_MENUBAR_ITEM_H
#include "editor/window/menubar/menubaritem.h"

// ── File / Save Scene ─────────────────────────────────────────────────────────
// Saves to the known path immediately. If no path is known (new unsaved scene),
// falls through to the Save As dialog automatically.

class MSaveSceneMenubarItem : public MMenubarItem
{
public:
    [[nodiscard]] int     getPriority() const override;
    [[nodiscard]] SString getPath()     const override;
    void onSelect()  override;
    void drawPopup() override;   // Save As dialog, only shown when path is unknown

private:
    static bool registered;
};

// ── File / Save Scene As ──────────────────────────────────────────────────────
// Always opens the file browser regardless of whether a path is already set.

class MSaveSceneAsMenubarItem : public MMenubarItem
{
public:
    [[nodiscard]] int     getPriority() const override;
    [[nodiscard]] SString getPath()     const override;
    void onSelect()  override;
    void drawPopup() override;

private:
    static bool registered;
};

#endif // SAVE_SCENE_MENUBAR_ITEM_H